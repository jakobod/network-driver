/**
 *  @author    Jakob Otto
 *  @file      detail/epoll_multiplexer.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#if defined(__linux__)

#  include "net/detail/epoll_multiplexer.hpp"

#  include "net/detail/event_handler.hpp"

#  include "net/event_result.hpp"
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
#  include <chrono>
#  include <iostream>
#  include <unistd.h>
#  include <utility>

namespace net::detail {

epoll_multiplexer::~epoll_multiplexer() {
  LOG_TRACE();
  ::close(mpx_fd_);
}

util::error epoll_multiplexer::init(manager_factory factory,
                                    const util::config& cfg) {
  LOG_TRACE();
  LOG_DEBUG("initializing epoll epoll_multiplexer");
  set_thread_id(std::this_thread::get_id());
  mpx_fd_ = epoll_create1(EPOLL_CLOEXEC);
  if (mpx_fd_ < 0) {
    return {util::error_code::runtime_error,
            "[epoll_multiplexer]: Creating epoll fd failed"};
  }
  LOG_DEBUG("Created ", NET_ARG(mpx_fd_));

  // TODO how to fix this sequence problem?
  if (auto err = multiplexer_base::init<event_handler>(
        [factory = std::move(factory), this](net::socket handle)
          -> manager_base_ptr { return factory(handle, this); },
        cfg)) {
    return err;
  }

  set_thread_id();
  return util::none;
}

using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::chrono::time_point_cast;

// -- Interface functions ------------------------------------------------------

void epoll_multiplexer::add(manager_base_ptr mgr, operation initial) {
  mgr->mask_set(initial);
  mod(mgr->handle().id, EPOLL_CTL_ADD, mgr->mask());
  multiplexer_base::add(mgr);
  if (auto err = mgr->init(cfg())) {
    handle_error(err);
  }
}

void epoll_multiplexer::enable(manager_base& mgr, operation op) {
  if (!mgr.mask_add(op)) {
    return;
  }
  mod(mgr.handle().id, EPOLL_CTL_MOD, mgr.mask());
}

void epoll_multiplexer::disable(manager_base& mgr, operation op, bool remove) {
  if (!mgr.mask_del(op)) {
    return;
  }
  mod(mgr.handle().id, EPOLL_CTL_MOD, mgr.mask());
  if (remove && (mgr.mask() == operation::none)) {
    del(mgr.handle());
  }
}

void epoll_multiplexer::del(socket handle) {
  mod(handle.id, EPOLL_CTL_DEL, operation::none);
  multiplexer_base::del(handle);
  if (shutting_down_ && !multiplexer_base::has_managers()) {
    running_ = false;
  }
}

epoll_multiplexer::manager_map::iterator
epoll_multiplexer::del(manager_map::iterator it) {
  auto fd = it->second->handle().id;
  mod(fd, EPOLL_CTL_DEL, operation::none);
  auto new_it = multiplexer_base::del(it);
  if (shutting_down_ && multiplexer_base::has_managers()) {
    running_ = false;
  }
  return new_it;
}

void epoll_multiplexer::mod(int fd, int op, operation events) {
  static const auto to_epoll_flag = [](net::operation op) -> uint32_t {
    switch (op) {
      case net::operation::none:
        return 0;
      case net::operation::read:
        return EPOLLIN;
      case net::operation::write:
        return EPOLLOUT;
      case net::operation::read_write:
        return (EPOLLIN | EPOLLOUT);
      default:
        return 0;
    }
  };
  epoll_event event{};
  event.events = to_epoll_flag(events);
  event.data.fd = fd;
  if (epoll_ctl(mpx_fd_, op, fd, &event) < 0) {
    handle_error({util::error_code::runtime_error, "epoll_ctl: {0}",
                  util::last_error_as_string()});
  }
}

util::error epoll_multiplexer::poll_once(bool blocking) {
  // Calculates the timeout value for the epoll_wait call
  const auto now = time_point_cast<milliseconds>(steady_clock::now());
  const auto timeout_passed = current_timeout_ ? (now > *current_timeout_)
                                               : false;
  const auto must_calculate_timeout = (blocking && current_timeout_.has_value()
                                       && !timeout_passed);
  int timeout = 0;
  if (must_calculate_timeout) {
    auto now = time_point_cast<milliseconds>(steady_clock::now());
    auto timeout_tp = time_point_cast<milliseconds>(*current_timeout_);
    timeout = static_cast<int>((timeout_tp - now).count());
  }

  // Poll for events on the reqistered sockets
  auto num_events = epoll_wait(mpx_fd_, pollset_.data(),
                               static_cast<int>(pollset_.size()), timeout);
  // Check for errors
  if (num_events < 0) {
    return (errno == EINTR) ? util::none
                            : util::error{util::error_code::runtime_error,
                                          util::last_error_as_string()};
  }
  // Handle all timeouts and io-events that have been registered
  handle_timeouts();
  handle_events(event_span(pollset_.data(), static_cast<size_t>(num_events)));
  return util::none;
}

void epoll_multiplexer::handle_events(event_span events) {
  auto handle_result = [&](manager_base& mgr, event_result res,
                           operation op) -> bool {
    if (res == event_result::done) {
      disable(mgr, op, true);
    } else if (res == event_result::error) {
      del(mgr.handle());
      return false;
    }
    return true;
  };

  for (auto& event : events) {
    if (event.events == (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
      LOG_ERROR("epoll_wait failed on socket = ", event.data.fd, ": ",
                util::last_error_as_string());
      del(socket{event.data.fd});
      continue;
    } else {
      auto* mgr = manager<event_handler>(net::socket{event.data.fd});
      if (!mgr) {
        LOG_ERROR("manager not found! This should never happen!");
        continue;
      }
      // Handle possible read event
      if ((event.events & EPOLLIN) == EPOLLIN) {
        if (!handle_result(*mgr, mgr->handle_read_event(), operation::read)) {
          continue;
        }
      }
      // Handle possible write event
      if ((event.events & EPOLLOUT) == EPOLLOUT) {
        if (!handle_result(*mgr, mgr->handle_write_event(), operation::read)) {
          continue;
        }
      }
    }
  }
}

} // namespace net::detail

#endif
