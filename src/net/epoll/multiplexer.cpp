/**
 *  @author    Jakob Otto
 *  @file      epoll/multiplexer.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#if !defined(__linux__)
#  error "epoll multiplexer is only usable on Linux"
#else

#  include "net/epoll/multiplexer.hpp"

#  include "net/acceptor.hpp"
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

namespace net::epoll {

multiplexer::~multiplexer() {
  LOG_TRACE();
  ::close(mpx_fd_);
}

util::error multiplexer::init(acceptor::factory_type factory,
                              const util::config& cfg) {
  LOG_TRACE();
  LOG_DEBUG("initializing epoll multiplexer");
  mpx_fd_ = epoll_create1(EPOLL_CLOEXEC);
  if (mpx_fd_ < 0) {
    return {util::error_code::runtime_error,
            "[multiplexer]: Creating epoll fd failed"};
  }
  LOG_DEBUG("Created ", NET_ARG(mpx_fd_));
  return multiplexer_base::init(std::move(factory), cfg);
}

using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::chrono::time_point_cast;

// -- Interface functions ------------------------------------------------------

void multiplexer::add(manager_base_ptr mgr, operation initial) {
  mgr->mask_set(initial);
  if (!nonblocking(mgr->handle(), true)) {
    handle_error(util::error(util::error_code::socket_operation_failed,
                             "Could not set nonblocking"));
  }
  mod(mgr->handle().id, EPOLL_CTL_ADD, mgr->mask());
  managers_.emplace(mgr->handle().id, mgr);
  // TODO: This should probably return an error instead of calling handle_error
  if (auto err = mgr->init(*cfg_)) {
    handle_error(err);
  }
}

void multiplexer::enable(manager_base& mgr, operation op) {
  if (!mgr.mask_add(op)) {
    return;
  }
  mod(mgr.handle().id, EPOLL_CTL_MOD, mgr.mask());
}

void multiplexer::disable(manager_base& mgr, operation op, bool remove) {
  if (!mgr.mask_del(op)) {
    return;
  }
  mod(mgr.handle().id, EPOLL_CTL_MOD, mgr.mask());
  if (remove && (mgr.mask() == operation::none)) {
    del(mgr.handle());
  }
}

void multiplexer::del(socket handle) {
  mod(handle.id, EPOLL_CTL_DEL, operation::none);
  managers_.erase(handle.id);
  if (shutting_down_ && managers_.empty()) {
    running_ = false;
  }
}

multiplexer::manager_map::iterator multiplexer::del(manager_map::iterator it) {
  auto fd = it->second->handle().id;
  mod(fd, EPOLL_CTL_DEL, operation::none);
  auto new_it = managers_.erase(it);
  if (shutting_down_ && managers_.empty()) {
    running_ = false;
  }
  return new_it;
}

void multiplexer::mod(int fd, int op, operation events) {
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

util::error multiplexer::poll_once(bool blocking) {
  // Calculates the timeout value for the epoll_wait call
  auto timeout = [&]() -> int {
    if (!blocking) {
      return 0; // nonblocking
    }
    if (!current_timeout_.has_value()) {
      return -1; // No timeout
    }
    auto now = time_point_cast<milliseconds>(steady_clock::now());
    auto timeout_tp = time_point_cast<milliseconds>(*current_timeout_);
    return static_cast<int>((timeout_tp - now).count());
  };

  // Poll for events on the reqistered sockets
  auto num_events = epoll_wait(mpx_fd_, pollset_.data(),
                               static_cast<int>(pollset_.size()), timeout());
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

void multiplexer::handle_events(event_span events) {
  auto handle_result = [&](manager_base_ptr& mgr, event_result res,
                           operation op) -> bool {
    if (res == event_result::done) {
      disable(*mgr, op, true);
    } else if (res == event_result::error) {
      del(mgr->handle());
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
      auto& mgr = managers_[event.data.fd];
      // Handle possible read event
      if ((event.events & EPOLLIN) == EPOLLIN) {
        if (!handle_result(mgr, mgr->handle_read_event(), operation::read)) {
          continue;
        }
      }
      // Handle possible write event
      if ((event.events & EPOLLOUT) == EPOLLOUT) {
        if (!handle_result(mgr, mgr->handle_write_event(), operation::write)) {
          continue;
        }
      }
    }
  }
}

util::error_or<multiplexer_ptr> make_multiplexer(acceptor::factory_type factory,
                                                 const util::config& cfg) {
  LOG_TRACE();
  auto mpx = std::make_shared<multiplexer>();
  if (auto err = mpx->init(std::move(factory), cfg)) {
    return err;
  }
  return mpx;
}

} // namespace net::epoll

#endif
