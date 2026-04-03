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

#  include "net/detail/acceptor.hpp"
#  include "net/detail/pollset_updater.hpp"

#  include "net/socket/socket.hpp"
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
#  include <csignal>
#  include <cstring>
#  include <iostream>
#  include <sys/socket.h>
#  include <unistd.h>
#  include <utility>

namespace {

struct submission_data {
  net::detail::uring_manager_ptr mgr;
  net::operation op;
};

io_uring_sqe* prepare_sqe(io_uring& uring, net::detail::uring_manager_ptr& mgr,
                          net::operation op) {
  auto* sqe = io_uring_get_sqe(&uring);
  if (!sqe) {
    LOG_ERROR("SQ ring full, cannot submit read operation for fd=",
              mgr->handle().id);
    return nullptr;
  }
  switch (op) {
    case net::operation::read: {
      auto& buf = mgr->read_buffer_for_submission();
      io_uring_prep_read(sqe, mgr->handle().id, buf.data(), buf.size(), 0);
      break;
    }

    case net::operation::write: {
      auto& buf = mgr->write_buffer_for_submission();
      io_uring_prep_write(sqe, mgr->handle().id, buf.data(), buf.size(), 0);
      break;
    }

    case net::operation::accept: {
      io_uring_prep_accept(sqe, mgr->handle().id, nullptr, nullptr, 0);
      break;
    }
    default:
      break;
  }
  return sqe;
}

void submit_operation(io_uring& uring, net::detail::uring_manager_ptr mgr,
                      net::operation op) {
  if (auto* sqe = prepare_sqe(uring, mgr, op)) {
    io_uring_sqe_set_data(sqe, new submission_data{std::move(mgr), op});
  }
}

void resubmit_operation(io_uring& uring, submission_data* data) {
  if (auto* sqe = prepare_sqe(uring, data->mgr, data->op)) {
    io_uring_sqe_set_data(sqe, data);
  }
}

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

  initialized_ = true;
  set_thread_id();
  return util::none;
}

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
    write_to_pipe(pollset_updater<uring_manager>::opcode::add, mgr.release(),
                  initial);
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
  if (!mgr.mask_add(op)) {
    return;
  }
  auto& uring_mgr = dynamic_cast<uring_manager&>(mgr);
  if ((op & operation::read) == operation::read) {
    submit_operation(uring_, &uring_mgr, operation::read);
  }

  if ((op & operation::write) == operation::write) {
    submit_operation(uring_, &uring_mgr, operation::write);
  }

  if ((op & operation::accept) == operation::accept) {
    submit_operation(uring_, &uring_mgr, operation::accept);
  }
}

void uring_multiplexer::disable(manager_base& mgr, operation op, bool remove) {
  static const auto to_shutdown_type = [](operation op) {
    switch (op) {
      case operation::read:
      case operation::accept:
      case operation::read_accept:
        return SHUT_RD;

      case operation::write:
        return SHUT_WR;

      case operation::read_write:
        return SHUT_RDWR;

      default:
        std::abort();
        return SHUT_RDWR;
    }
  };

  LOG_TRACE();
  LOG_DEBUG("Disabling mgr with ", NET_ARG2("id", mgr->handle().id),
            " registered for ", NET_ARG2("mask", to_string(mgr->mask())),
            " for ", NET_ARG2("event", to_string(op)));
  if (!mgr.mask_del(op)) {
    return;
  }
  ::net::shutdown(mgr.handle(), to_shutdown_type(op));
  if (remove && (mgr.mask() == operation::none)) {
    multiplexer_base::del(mgr.handle());
  }
}

util::error uring_multiplexer::poll_once(bool blocking) {
  using namespace std::chrono;
  LOG_TRACE();

  if (!initialized_) {
    LOG_ERROR("uring_multiplexer not initialized, cannot poll");
    return {util::error_code::runtime_error,
            "[uring_multiplexer]: not initialized"};
  }

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

    // Handle the completion based on operation type
    if (cqe->res < 0) {
      LOG_ERROR("I/O operation failed: ", util::last_error_as_string());
      multiplexer_base::del(data->mgr->handle());
    } else {
      // Successful completion - pass to manager
      auto result = data->mgr->handle_completion(data->op, cqe->res);
      switch (result) {
        case event_result::ok:
          resubmit_operation(uring_, data);
          data = nullptr;
          break;
        case event_result::done:
          disable(*data->mgr, data->op, true);
          break;
        case event_result::error:
          multiplexer_base::del(data->mgr->handle());
          break;
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
