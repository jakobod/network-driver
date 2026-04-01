/**
 *  @author    Jakob Otto
 *  @file      epoll/multiplexer.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/epoll/multiplexer.hpp"

#include "net/acceptor.hpp"
#include "net/event_result.hpp"
#include "net/operation.hpp"
#include "net/pollset_updater.hpp"

#include "net/socket/pipe_socket.hpp"
#include "net/socket/tcp_accept_socket.hpp"

#include "net/ip/v4_address.hpp"
#include "net/ip/v4_endpoint.hpp"

#include "util/binary_serializer.hpp"
#include "util/byte_span.hpp"
#include "util/config.hpp"
#include "util/error.hpp"
#include "util/error_or.hpp"
#include "util/format.hpp"
#include "util/intrusive_ptr.hpp"
#include "util/logger.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <unistd.h>
#include <utility>

namespace net::epoll {

multiplexer::~multiplexer() {
  LOG_TRACE();
  ::close(mpx_fd_);
}

util::error multiplexer::init(acceptor::factory_type factory,
                              const util::config& cfg) {
  LOG_TRACE();
  set_thread_id(std::this_thread::get_id());
  cfg_ = std::addressof(cfg);
  LOG_DEBUG("initializing epoll multiplexer");
  mpx_fd_ = epoll_create1(EPOLL_CLOEXEC);
  if (mpx_fd_ < 0) {
    return {util::error_code::runtime_error,
            "[multiplexer]: Creating epoll fd failed"};
  }
  LOG_DEBUG("Created ", NET_ARG(mpx_fd_));
  // Create pollset updater
  auto pipe_res = make_pipe();
  if (auto err = util::get_error(pipe_res)) {
    return *err;
  }
  auto pipe_fds = std::get<pipe_socket_pair>(pipe_res);
  pipe_reader_ = pipe_fds.first;
  pipe_writer_ = pipe_fds.second;
  add(util::make_intrusive<pollset_updater>(pipe_reader_, this),
      operation::read);
  // Create Acceptor
  auto res = net::make_tcp_accept_socket(ip::v4_endpoint(
    (cfg_->get_or("multiplexer.local", true) ? ip::v4_address::localhost
                                             : ip::v4_address::any),
    cfg_->get_or<std::int64_t>("multiplexer.port", 0)));
  if (auto err = util::get_error(res)) {
    return *err;
  }
  auto accept_socket_pair = std::get<net::acceptor_pair>(res);
  auto accept_socket = accept_socket_pair.first;
  set_port(accept_socket_pair.second);
  add(util::make_intrusive<acceptor>(accept_socket, this, std::move(factory)),
      operation::read);
  set_thread_id();
  return util::none;
}

void multiplexer::start() {
  LOG_TRACE();
  if (!running_) {
    running_ = true;
    mpx_thread_ = std::thread(&multiplexer::run, this);
    mpx_thread_id_ = mpx_thread_.get_id();
    LOG_DEBUG(NET_ARG(mpx_thread_id_));
  }
}

void multiplexer::shutdown() {
  LOG_TRACE();
  if (is_multiplexer_thread()) {
    LOG_DEBUG("multiplexer shutting down");
    auto it = managers_.begin();
    while (it != managers_.end()) {
      auto& mgr = it->second;
      if (mgr->handle() != pipe_reader_) {
        disable(*mgr, operation::read, false);
        if (mgr->mask() == operation::none) {
          it = del(it);
        } else {
          ++it;
        }
      } else {
        ++it;
      }
    }
    shutting_down_ = true;
    close(pipe_writer_);
    pipe_writer_ = pipe_socket{};
  } else if (!shutting_down_) {
    LOG_DEBUG("requesting multiplexer shutdown");
    auto res = write_to_pipe(pollset_updater::shutdown_code);
    if (res != 1) {
      LOG_ERROR("could not write shutdown code to pipe: ",
                last_socket_error_as_string());
      std::terminate(); // Can't be handled by shutting down, if shutdown fails.
    }
  }
}

void multiplexer::join() {
  LOG_TRACE();
  if (mpx_thread_.joinable()) {
    LOG_DEBUG("joining on multiplexer thread");
    mpx_thread_.join();
  }
}

bool multiplexer::is_running() const noexcept {
  return mpx_thread_.joinable();
}

void multiplexer::set_thread_id(std::thread::id tid) noexcept {
  mpx_thread_id_ = tid;
}

void multiplexer::run() {
  LOG_TRACE();
  while (running_) {
    if (poll_once(true)) {
      running_ = false;
    }
  }
}

// -- Error handling -----------------------------------------------------------

void multiplexer::handle_error([[maybe_unused]] const util::error& err) {
  LOG_ERROR(err);
  shutdown();
}

// -- Timeout management -------------------------------------------------------

uint64_t multiplexer::set_timeout(manager_base_ptr mgr,
                                  std::chrono::steady_clock::time_point when) {
  LOG_TRACE();
  LOG_DEBUG("Setting timeout ", current_timeout_id_, " on ",
            NET_ARG2("mgr", mgr->handle().id));
  timeouts_.emplace(mgr->handle().id, when, current_timeout_id_);
  current_timeout_ = (current_timeout_ != std::nullopt)
                       ? std::min(when, *current_timeout_)
                       : when;
  return current_timeout_id_++;
}

void multiplexer::handle_timeouts() {
  using namespace std::chrono;
  LOG_TRACE();
  const auto now = steady_clock::now();
  for (auto it = timeouts_.begin(); it != timeouts_.end(); ++it) {
    const auto& entry = *it;
    const auto when = time_point_cast<milliseconds>(entry.when_);
    if (when <= now) {
      // Registered timeout has expired
      auto& mgr = *managers_.at(entry.handle_);
      mgr.handle_timeout(entry.id_);
    } else {
      // Timeout not expired, delete handled entries and set the current timeout
      timeouts_.erase(timeouts_.begin(), it);
      if (timeouts_.empty()) {
        LOG_DEBUG("No further timeouts registered");
        current_timeout_ = std::nullopt;
      } else {
        LOG_DEBUG("Next timeout with ", NET_ARG2("id", entry.id_));
        current_timeout_ = entry.when_;
      }
      break;
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

} // namespace net::epoll
