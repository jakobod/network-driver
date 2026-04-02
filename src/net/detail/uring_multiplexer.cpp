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

namespace {

struct submission_data {
  net::detail::uring_manager_ptr mgr;
  net::operation op;
};

void submit_read(io_uring& uring, net::detail::uring_manager_ptr mgr) {
  auto* sqe = io_uring_get_sqe(&uring);
  auto& buf = mgr->read_buffer_for_submission();
  io_uring_prep_read(sqe, mgr->handle().id, buf.data(), buf.size(), 0);
  auto* data = new submission_data{std::move(mgr), net::operation::read};
  io_uring_sqe_set_data(sqe, data);
}

void submit_write(io_uring& uring, net::detail::uring_manager_ptr mgr) {
  auto* sqe = io_uring_get_sqe(&uring);
  auto& buf = mgr->write_buffer_for_submission();
  io_uring_prep_write(sqe, mgr->handle().id, buf.data(), buf.size(), 0);
  auto* data = new submission_data{std::move(mgr), net::operation::write};
  io_uring_sqe_set_data(sqe, data);
}

void submit_accept(io_uring& uring, net::detail::uring_manager_ptr mgr) {
  auto* sqe = io_uring_get_sqe(&uring);
  auto& buf = mgr->write_buffer_for_submission();
  io_uring_prep_write(sqe, mgr->handle().id, buf.data(), buf.size(), 0);
  auto* data = new submission_data{std::move(mgr), net::operation::accept};
  io_uring_sqe_set_data(sqe, data);
}

} // namespace

namespace net::detail {

uring_multiplexer::~uring_multiplexer() {
  LOG_TRACE();
  io_uring_queue_exit(&uring_);
}

util::error uring_multiplexer::init(manager_factory factory,
                                    const util::config& cfg) {
  LOG_TRACE();
  LOG_DEBUG("initializing uring_multiplexer");
  if (auto res = io_uring_queue_init(max_uring_depth, &uring_, 0); res < 0) {
    fprintf(stderr, "io_uring_queue_init failed: %s\n", strerror(-res));
    return {util::error_code::runtime_error,
            "[uring_multiplexer]: initializing uring failed"};
  }
  LOG_DEBUG("Created io_uring with depth ", NET_ARG(max_uring_depth));

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
    auto [_, success] = managers_.emplace(mgr->handle().id, mgr);
    if (!success) {
      handle_error(util::error{util::error_code::runtime_error,
                               "Failed to emplace manager"});
    }
    if (auto err = mgr->init(*cfg_)) {
      handle_error(err);
    }
    enable(*mgr, initial);
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
  LOG_DEBUG("Deleting mgr with ", NET_ARG2("id", it->second->handle().id));
  auto new_it = managers_.erase(it);
  if (shutting_down_ && managers_.empty()) {
    running_ = false;
  }
  return new_it;
}

void uring_multiplexer::enable(manager_base& mgr, operation op) {
  LOG_TRACE();
  LOG_DEBUG("Enabling mgr with ", NET_ARG2("id", mgr->handle().id),
            " registered for ", NET_ARG2("mask", to_string(mgr->mask())),
            " for ", NET_ARG2("new_event", to_string(op)));
  auto& uring_mgr = static_cast<uring_manager&>(mgr);
  if ((op & operation::read) == operation::read) {
    submit_read(uring_, &uring_mgr);
  }

  if ((op & operation::write) == operation::write) {
    submit_write(uring_, &uring_mgr);
  }

  if ((op & operation::accept) == operation::accept) {
    submit_accept(uring_, &uring_mgr);
  }
}

void uring_multiplexer::disable([[maybe_unused]] manager_base& mgr,
                                [[maybe_unused]] operation op,
                                [[maybe_unused]] bool remove) {
  LOG_TRACE();
  LOG_DEBUG("Disabling mgr with ", NET_ARG2("id", mgr->handle().id),
            " registered for ", NET_ARG2("mask", to_string(mgr->mask())),
            " for ", NET_ARG2("event", to_string(op)));
}

util::error uring_multiplexer::poll_once(bool blocking) {
  using namespace std::chrono;
  LOG_TRACE();

  // Submit any pending operations
  io_uring_submit(&uring_);

  const auto now = time_point_cast<milliseconds>(steady_clock::now());
  const auto timeout_passed = (current_timeout_ ? (now > *current_timeout_)
                                                : false);
  // Calculate the timeout value for the kqueue call
  __kernel_timespec timeout{0, 0};

  if (blocking && current_timeout_ && !timeout_passed) {
    const auto timeout_tp = time_point_cast<milliseconds>(*current_timeout_);
    const auto diff_ms = (timeout_tp - now).count();
    timeout.tv_nsec = (diff_ms / 1000);
    timeout.tv_nsec = ((diff_ms % 1000) * 1000000);
  }

  // Pass nullptr only when blocking indefinitely (no timeout set)
  // Otherwise pass &timeout: either {0, 0} for non-blocking/passed timeout or
  // actual timeout
  auto* timeout_ptr = (blocking && !current_timeout_) ? nullptr : &timeout;
  // Wait for completion events with timeout
  io_uring_cqe* cqe = nullptr;
  int ret = io_uring_wait_cqe_timeout(&uring_, &cqe, timeout_ptr);

  if (ret < 0 && ret != -ETIME) {
    return {util::error_code::runtime_error,
            util::format("io_uring_wait_cqe_timeout failed: {0}",
                         util::last_error_as_string())};
  }

  // Handle all timeouts and io-events that have been registered
  handle_timeouts();
  handle_events();
  return util::none;
}

void uring_multiplexer::handle_events() {
  LOG_TRACE();
  io_uring_cqe* cqe;
  unsigned head;
  unsigned count = 0;

  // Iterate through all available completion events
  io_uring_for_each_cqe(&uring_, head, cqe) {
    auto* data = reinterpret_cast<submission_data*>(io_uring_cqe_get_data(cqe));
    if (!data) {
      LOG_WARNING("Received CQE with null user_data");
      ++count;
      continue;
    }

    LOG_DEBUG("Handling CQE for fd=", data->mgr->handle().id,
              " op=", to_string(data->op), " res=", cqe->res);

    // Handle the completion based on operation type
    if (cqe->res < 0) {
      LOG_ERROR("I/O operation failed: ", util::last_error_as_string());
      auto it = managers_.find(data->mgr->handle().id);
      if (it != managers_.end()) {
        del(it);
      }
    } else {
      // Successful completion - pass to manager
      auto result = data->mgr->handle_completion(data->op, cqe->res);
      if (result == event_result::ok) {
        enable(*data->mgr, data->op);
      } else if (result == event_result::error) {
        auto it = managers_.find(data->mgr->handle().id);
        if (it != managers_.end()) {
          del(it);
        }
      }
    }

    count++;
    delete data;
  }

  // Mark all CQEs as consumed
  io_uring_cq_advance(&uring_, count);
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
