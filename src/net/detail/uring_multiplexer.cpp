/**
 *  @author    Jakob Otto
 *  @file      uring_multiplexer.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released
 under
 *             the GNU GPL3 License.
 */

#if defined(LIB_NET_URING)

#  include "net/detail/uring_multiplexer.hpp"

#  include "net/socket/socket.hpp"
#  include "net/socket/tcp_accept_socket.hpp"

#  include "net/ip/v4_address.hpp"
#  include "net/ip/v4_endpoint.hpp"
#  include "net/manager_result.hpp"
#  include "net/operation.hpp"

#  include "util/assert.hpp"
#  include "util/binary_serializer.hpp"
#  include "util/byte_span.hpp"
#  include "util/config.hpp"
#  include "util/error.hpp"
#  include "util/error_or.hpp"
#  include "util/format.hpp"
#  include "util/intrusive_ptr.hpp"
#  include "util/logger.hpp"

#  include <algorithm>
#  include <csignal>
#  include <cstring>
#  include <iostream>
#  include <poll.h>
#  include <sys/socket.h>
#  include <unistd.h>
#  include <utility>

namespace {

struct submission_data {
  net::detail::uring_manager_ptr mgr;
  net::operation op;
  std::uint64_t id;
  bool multishot;
};

} // namespace

namespace net::detail {

uring_multiplexer::~uring_multiplexer() {
  LOG_TRACE();
  if (initialized_) {
    io_uring_queue_exit(&uring_);
  }
}

util::error uring_multiplexer::init(manager_factory factory,
                                    const util::config& cfg) {
  LOG_TRACE();
  if (initialized_) {
    return util::error{util::error_code::runtime_error,
                       "multiplexer_base was already initialized"};
  }
  LOG_DEBUG("initializing uring_multiplexer");
  set_thread_id(std::this_thread::get_id());

  if (auto res = io_uring_queue_init(max_uring_depth, &uring_, 0); res < 0) {
    fprintf(stderr, "io_uring_queue_init failed: %s\n", strerror(-res));
    return {util::error_code::runtime_error,
            "[uring_multiplexer]: initializing uring failed"};
  }
  LOG_DEBUG("Created io_uring with depth ", NET_ARG(max_uring_depth));

  // TODO how to fix this sequence problem?
  if (auto err = multiplexer_base::init<uring_manager>(
        [factory = std::move(factory), this](net::socket handle)
          -> manager_base_ptr { return factory(handle, this); },
        cfg)) {
    return err;
  }

  set_thread_id();
  return util::none;
}

// -- IO Operation submission --------------------------------------------------

io_uring_sqe* uring_multiplexer::prepare_submission(uring_manager_ptr mgr,
                                                    operation op,
                                                    bool multishot) {
  if (auto* sqe = io_uring_get_sqe(&uring_)) {
    io_uring_sqe_set_data(sqe, new submission_data{std::move(mgr), op,
                                                   current_submission_id_,
                                                   multishot});
    return sqe;
  }
  return nullptr;
}

std::pair<bool, uint64_t> uring_multiplexer::submit_accept(uring_manager& mgr,
                                                           bool multishot) {
  if (auto* sqe = prepare_submission(as_intrusive_ptr(mgr), operation::accept,
                                     multishot)) {
    if (multishot) [[unlikely]] {
      io_uring_prep_multishot_accept(sqe, mgr.handle().id, nullptr, nullptr, 0);
    } else [[likely]] {
      io_uring_prep_accept(sqe, mgr.handle().id, nullptr, nullptr, 0);
    }
    return {true, current_submission_id_++};
  }
  return {false, 0};
}

std::pair<bool, uint64_t>
uring_multiplexer::submit_poll_read(uring_manager& mgr, bool multishot) {
  if (auto* sqe = prepare_submission(as_intrusive_ptr(mgr),
                                     operation::poll_read, multishot)) {
    if (multishot) [[unlikely]] {
      io_uring_prep_poll_multishot(sqe, mgr.handle().id, POLLIN);
    } else [[likely]] {
      io_uring_prep_poll_add(sqe, mgr.handle().id, POLLIN);
    }
    return {true, current_submission_id_++};
  }
  return {false, 0};
}

std::pair<bool, uint64_t>
uring_multiplexer::submit_poll_write(uring_manager& mgr, bool multishot) {
  if (auto* sqe = prepare_submission(as_intrusive_ptr(mgr),
                                     operation::poll_write, multishot)) {
    if (multishot) [[unlikely]] {
      io_uring_prep_poll_multishot(sqe, mgr.handle().id, POLLOUT);
    } else [[likely]] {
      io_uring_prep_poll_add(sqe, mgr.handle().id, POLLOUT);
    }
    return {true, current_submission_id_++};
  }
  return {false, 0};
}

std::pair<bool, uint64_t>
uring_multiplexer::submit_read(uring_manager& mgr,
                               util::byte_span read_buffer) {
  if (auto* sqe = prepare_submission(as_intrusive_ptr(mgr), operation::read)) {
    io_uring_prep_read(sqe, mgr.handle().id,
                       static_cast<void*>(read_buffer.data()),
                       read_buffer.size(), 0);
    return {true, current_submission_id_++};
  }
  return {false, 0};
}

std::pair<bool, uint64_t>
uring_multiplexer::submit_write(uring_manager& mgr,
                                util::byte_span write_buffer) {
  if (auto* sqe = prepare_submission(as_intrusive_ptr(mgr), operation::write)) {
    io_uring_prep_write(sqe, mgr.handle().id,
                        static_cast<void*>(write_buffer.data()),
                        write_buffer.size(), 0);
    return {true, current_submission_id_++};
  }
  return {false, 0};
}

std::pair<bool, uint64_t>
uring_multiplexer::submit_readv(uring_manager& mgr,
                                std::span<iovec> read_vecs) {
  if (auto* sqe = prepare_submission(as_intrusive_ptr(mgr), operation::read)) {
    io_uring_prep_readv(sqe, mgr.handle().id, read_vecs.data(),
                        read_vecs.size(), 0);
    return {true, current_submission_id_++};
  }
  return {false, 0};
}

std::pair<bool, uint64_t>
uring_multiplexer::submit_writev(uring_manager& mgr,
                                 std::span<iovec> write_vecs) {
  if (auto* sqe = prepare_submission(as_intrusive_ptr(mgr), operation::write)) {
    io_uring_prep_writev(sqe, mgr.handle().id, write_vecs.data(),
                         write_vecs.size(), 0);
    return {true, current_submission_id_++};
  }
  return {false, 0};
}

std::pair<bool, uint64_t> uring_multiplexer::submit_recvmsg(uring_manager& mgr,
                                                            msghdr& read_msghdr,
                                                            bool multishot) {
  if (auto* sqe = prepare_submission(as_intrusive_ptr(mgr), operation::read,
                                     multishot)) {
    if (multishot) [[unlikely]] {
      io_uring_prep_recvmsg_multishot(sqe, mgr.handle().id, &read_msghdr, 0);
    } else [[likely]] {
      io_uring_prep_recvmsg(sqe, mgr.handle().id, &read_msghdr, 0);
    }
    return {true, current_submission_id_++};
  }
  return {false, 0};
}

std::pair<bool, uint64_t>
uring_multiplexer::submit_sendmsg(uring_manager& mgr, msghdr& write_msghdr) {
  if (auto* sqe = prepare_submission(as_intrusive_ptr(mgr), operation::write)) {
    io_uring_prep_sendmsg(sqe, mgr.handle().id, &write_msghdr, 0);
    return {true, current_submission_id_++};
  }
  return {false, 0};
}

// -- Interface functions ------------------------------------------------------

void uring_multiplexer::add(manager_base_ptr mgr, operation initial) {
  LOG_TRACE();
  if (is_multiplexer_thread()) {
    LOG_DEBUG("Adding socket_manager with ", NET_ARG2("id", mgr->handle().id),
              " for ", NET_ARG(initial));
    auto& added_mgr = multiplexer_base::add(std::move(mgr));
    if (auto err = added_mgr->init(cfg())) {
      handle_error(err);
    }
    enable(*added_mgr, initial);
  } else {
    LOG_DEBUG("Requesting to add socket_manager with ",
              NET_ARG2("id", mgr->handle().id), " for ", NET_ARG(initial));
    write_to_pipe(pollset_opcode::add, mgr.release(), initial);
  }
}

uring_multiplexer::manager_map::iterator
uring_multiplexer::del(manager_map::iterator it) {
  LOG_TRACE();
  LOG_DEBUG("Deleting mgr with ", NET_ARG2("id", it->second->handle().id));
  auto new_it = multiplexer_base::del(it);
  if (shutting_down_ && multiplexer_base::has_managers()) {
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
  if (!uring_mgr.mask_add(op)) {
    return;
  }
  const auto enable_res = uring_mgr.enable(op);
  if (enable_res != manager_result::ok) {
    LOG_ERROR("Failed to enable uring_manager with ",
              NET_ARG2("hdl", mgr.handle().id));
    uring_mgr.mask_del(op);
  }
}

void uring_multiplexer::disable(manager_base& mgr, operation op, bool remove) {
  LOG_TRACE();
  LOG_DEBUG("Disabling mgr with ", NET_ARG2("id", mgr->handle().id),
            " registered for ", NET_ARG2("mask", to_string(mgr->mask())),
            " for ", NET_ARG2("event", to_string(op)));
  if (!mgr.mask_del(op)) {
    return;
  }
  if (remove && (mgr.mask() == operation::none)) {
    multiplexer_base::del(mgr.handle());
  }
}

util::error uring_multiplexer::poll_once(bool blocking) {
  using namespace std::chrono;
  LOG_TRACE();

  // Submit any pending operations
  int submit_res = io_uring_submit(&uring_);
  if (submit_res < 0) {
    return {util::error_code::runtime_error,
            util::format("io_uring_submit failed: {0}",
                         util::last_error_as_string())};
  }
  LOG_DEBUG("Submitted ", submit_res, " operations to io_uring");

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
      LOG_ERROR("Received CQE with null user_data");
      ++count;
      continue;
    }

    LOG_DEBUG("Handling CQE for fd=", data->mgr->handle().id,
              " op=", to_string(data->op), " res=", cqe->res);

    auto result = data->mgr->handle_completion(data->op, cqe->res, data->id);
    switch (result) {
      case manager_result::ok:
        break;
      case manager_result::temporary_error:
      case manager_result::done:
        disable(*data->mgr, data->op, true);
        break;
      case manager_result::error:
        multiplexer_base::del(data->mgr->handle());
        break;
    }

    if (!data->multishot) {
      delete data;
    }
    count++;
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
