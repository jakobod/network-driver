/**
 *  @author    Jakob Otto
 *  @file      uring_multiplexer.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released
 under
 *             the GNU GPL3 License.
 */

#if !defined(__linux__)
#  error "uring_multiplexer is only usable on Linux"
#elif !defined(LIB_NET_URING)
#  error "uring support not enabled"
#else

#  include "net/detail/uring_multiplexer.hpp"

#  include "net/detail/acceptor.hpp"
#  include "net/detail/pollset_updater.hpp"

#  include "net/socket/tcp_accept_socket.hpp"

#  include "net/event_result.hpp"
#  include "net/ip/v4_address.hpp"
#  include "net/ip/v4_endpoint.hpp"
#  include "net/operation.hpp"

#  include "util/binary_serializer.hpp"
#  include "util/byte_span.hpp"
#  include "util/config.hpp"
#  include "util/error.hpp"
#  include "util/error_or.hpp"
#  include "util/format.hpp"
#  include "util/intrusive_ptr.hpp"
#  include "util/logger.hpp"

#  include <algorithm>
#  include <iostream>
#  include <unistd.h>
#  include <utility>

namespace net::detail {

uring_multiplexer::~uring_multiplexer() {
  LOG_TRACE();
  io_uring_queue_exit(&uring_);
}

util::error uring_multiplexer::init(manager_factory factory,
                                    const util::config& cfg) {
  LOG_TRACE();
  LOG_DEBUG("initializing uring_multiplexer");
  if (io_uring_queue_init(max_uring_depth, &uring_, 0) < 0) {
    return {util::error_code::runtime_error,
            "[uring_multiplexer]: initializing uring failed"};
  }
  LOG_DEBUG("Created ", NET_ARG(mpx_fd_));

  // Create Acceptor
  const auto res = net::make_tcp_accept_socket(ip::v4_endpoint(
    (cfg_->get_or("multiplexer.local", true) ? ip::v4_address::localhost
                                             : ip::v4_address::any),
    cfg_->get_or<std::int64_t>("multiplexer.port", 0)));
  if (auto err = util::get_error(res)) {
    return *err;
  }
  auto accept_socket_pair = std::get<net::acceptor_pair>(res);
  auto accept_socket = accept_socket_pair.first;
  set_port(accept_socket_pair.second);

  auto generic_factory = [factory = std::move(factory),
                          this](net::socket handle) -> manager_base_ptr {
    return factory(handle, this);
  };

  add(util::make_intrusive<acceptor>(accept_socket, this,
                                     std::move(generic_factory)),
      operation::accept);

  return multiplexer_base::init(cfg);
}

void uring_multiplexer::add(manager_base_ptr mgr, operation initial) {
  LOG_TRACE();
  if (is_multiplexer_thread()) {
    LOG_DEBUG("Adding socket_manager with ", NET_ARG2("id", mgr->handle().id),
              " for ", NET_ARG(initial));
    // Set nonblocking on socket
    if (!nonblocking(mgr->handle(), true)) {
      handle_error(util::error(util::error_code::socket_operation_failed,
                               "Could not set nonblocking on handle"));
    }

  } else {
    LOG_DEBUG("Requesting to add socket_manager with ",
              NET_ARG2("id", mgr->handle().id), " for ", NET_ARG(initial));
    mgr->ref();
    write_to_pipe(pollset_updater::opcode::add, mgr.get(), initial);
  }
}

uring_multiplexer::manager_map::iterator
uring_multiplexer::del(manager_map::iterator it) {
  LOG_TRACE();
  LOG_DEBUG("Deleting mgr with ", NET_ARG2("id", fd));
  return std::next(it);
}

void uring_multiplexer::enable([[maybe_unused]] manager_base& mgr,
                               [[maybe_unused]] operation op) {
  LOG_TRACE();
  LOG_DEBUG("Enabling mgr with ", NET_ARG2("id", mgr->handle().id),
            " registered for ", NET_ARG2("mask", to_string(mgr->mask())),
            " for ", NET_ARG2("new_event", to_string(op)));
}

void uring_multiplexer::disable([[maybe_unused]] manager_base& mgr,
                                [[maybe_unused]] operation op,
                                [[maybe_unused]] bool remove) {
  LOG_TRACE();
  LOG_DEBUG("Disabling mgr with ", NET_ARG2("id", mgr->handle().id),
            " registered for ", NET_ARG2("mask", to_string(mgr->mask())),
            " for ", NET_ARG2("event", to_string(op)));
}

util::error uring_multiplexer::poll_once([[maybe_unused]] bool blocking) {
  using namespace std::chrono;
  LOG_TRACE();
  // Handle all timeouts and io-events that have been registered
  handle_timeouts();
  handle_events();
  return util::none;
}

void uring_multiplexer::handle_events() {
  LOG_TRACE();
  LOG_DEBUG("Handling ", events.size(), " I/O events");
}

util::error_or<uring_multiplexer_ptr>
make_uring_multiplexer(uring_multiplexer::manager_factory factory,
                       const util::config& cfg) {
  LOG_TRACE();
  auto mpx = std::make_shared<uring_multiplexer>();
  if (auto err = mpx->init(std::move(factory), cfg)) {
    return err;
  }
  return mpx;
}

} // namespace net::detail

#endif
