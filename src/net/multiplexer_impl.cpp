/**
 *  @author    Jakob Otto
 *  @file      multiplexer_impl.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/multiplexer_impl.hpp"

#include "net/acceptor.hpp"
#include "net/event_result.hpp"
#include "net/operation.hpp"
#include "net/pipe_socket.hpp"
#include "net/pollset_updater.hpp"
#include "net/socket_manager.hpp"
#include "net/tcp_accept_socket.hpp"

#include "util/binary_serializer.hpp"
#include "util/byte_span.hpp"
#include "util/error.hpp"
#include "util/error_or.hpp"
#include "util/format.hpp"
#include "util/intrusive_ptr.hpp"
#include "util/logger.hpp"

#include <algorithm>
#include <iostream>
#include <unistd.h>
#include <utility>

// -- identical implementation of the multiplexer_impl accross OSes ------------

namespace net {

multiplexer_impl::~multiplexer_impl() {
  LOG_TRACE();
  ::close(mpx_fd_);
}

util::error multiplexer_impl::init(socket_manager_factory_ptr factory,
                                   uint16_t port, bool local) {
  LOG_TRACE();
#if defined(EPOLL_MPX)
  LOG_DEBUG("initializing epoll multiplexer");
  mpx_fd_ = epoll_create1(EPOLL_CLOEXEC);
#elif defined(KQUEUE_MPX)
  LOG_DEBUG("initializing kqueue multiplexer");
  mpx_fd_ = kqueue();
#endif
  if (mpx_fd_ < 0)
    return {util::error_code::runtime_error,
            "[multiplexer_impl]: Creating epoll fd failed"};
  LOG_DEBUG("Created ", NET_ARG(mpx_fd_));
  // Create pollset updater
  auto pipe_res = make_pipe();
  if (auto err = util::get_error(pipe_res))
    return *err;
  auto pipe_fds = std::get<pipe_socket_pair>(pipe_res);
  pipe_reader_ = pipe_fds.first;
  pipe_writer_ = pipe_fds.second;
  add(util::make_intrusive<pollset_updater>(pipe_reader_, this),
      operation::read);
  // Create Acceptor
  auto res = net::make_tcp_accept_socket(
    {(local ? ip::v4_address::localhost : ip::v4_address::any), port});
  if (auto err = util::get_error(res))
    return *err;
  auto accept_socket_pair = std::get<net::acceptor_pair>(res);
  auto accept_socket = accept_socket_pair.first;
  port_ = accept_socket_pair.second;
  add(util::make_intrusive<acceptor>(accept_socket, this, std::move(factory)),
      operation::read);
  return util::none;
}

void multiplexer_impl::start() {
  LOG_TRACE();
  if (!running_) {
    running_ = true;
    mpx_thread_ = std::thread(&multiplexer_impl::run, this);
    mpx_thread_id_ = mpx_thread_.get_id();
  }
}

void multiplexer_impl::shutdown() {
  LOG_TRACE();
  if (is_multiplexer_thread()) {
    LOG_DEBUG("multiplexer shutting down");
    auto it = managers_.begin();
    while (it != managers_.end()) {
      auto& mgr = it->second;
      if (mgr->handle() != pipe_reader_) {
        disable(mgr, operation::read, false);
        if (mgr->mask() == operation::none)
          it = del(it);
        else
          ++it;
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

void multiplexer_impl::join() {
  LOG_TRACE();
  if (mpx_thread_.joinable()) {
    LOG_DEBUG("joining on multiplexer thread");
    mpx_thread_.join();
  }
}

bool multiplexer_impl::running() const {
  return mpx_thread_.joinable();
}

void multiplexer_impl::set_thread_id() {
  mpx_thread_id_ = std::this_thread::get_id();
}

void multiplexer_impl::run() {
  LOG_TRACE();
  while (running_) {
    if (poll_once(true))
      running_ = false;
  }
}

// -- Error handling -----------------------------------------------------------

void multiplexer_impl::handle_error([[maybe_unused]] const util::error& err) {
  LOG_ERROR(err);
  shutdown();
}

// -- Timeout management -------------------------------------------------------

uint64_t
multiplexer_impl::set_timeout(socket_manager_ptr mgr,
                              std::chrono::system_clock::time_point when) {
  LOG_TRACE();
  LOG_DEBUG("Setting timeout ", current_timeout_id_, " on ",
            NET_ARG2("mgr", mgr->handle().id));
  timeouts_.emplace(mgr->handle().id, when, current_timeout_id_);
  current_timeout_ = (current_timeout_ != std::nullopt)
                       ? std::min(when, *current_timeout_)
                       : when;
  return current_timeout_id_++;
}

void multiplexer_impl::handle_timeouts() {
  using namespace std::chrono;
  LOG_TRACE();
  const auto now = time_point_cast<milliseconds>(system_clock::now());
  for (auto it = timeouts_.begin(); it != timeouts_.end(); ++it) {
    const auto& entry = *it;
    const auto when = time_point_cast<milliseconds>(entry.when_);
    if (when <= now) {
      // Registered timeout has expired
      managers_.at(entry.handle_)->handle_timeout(entry.id_);
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

util::error_or<multiplexer_ptr>
make_multiplexer(socket_manager_factory_ptr factory, uint16_t port) {
  LOG_TRACE();
  auto mpx = std::make_shared<multiplexer_impl>();
  if (auto err = mpx->init(std::move(factory), port))
    return err;
  return mpx;
}

#if defined(EPOLL_MPX) // -- epoll specific implementation ---------------------

using std::chrono::milliseconds;
using std::chrono::system_clock;
using std::chrono::time_point_cast;

// -- Interface functions ------------------------------------------------------

void multiplexer_impl::add(socket_manager_ptr mgr, operation initial) {
  mgr->mask_set(initial);
  if (!nonblocking(mgr->handle(), true))
    handle_error(util::error(util::error_code::socket_operation_failed,
                             "Could not set nonblocking"));
  mod(mgr->handle().id, EPOLL_CTL_ADD, mgr->mask());
  managers_.emplace(mgr->handle().id, mgr);
  // TODO: This should probably return an error instead of calling handle_error
  if (auto err = mgr->init())
    handle_error(err);
}

void multiplexer_impl::enable(socket_manager_ptr mgr, operation op) {
  if (!mgr->mask_add(op))
    return;
  mod(mgr->handle().id, EPOLL_CTL_MOD, mgr->mask());
}

void multiplexer_impl::disable(socket_manager_ptr mgr, operation op,
                               bool remove) {
  if (!mgr->mask_del(op))
    return;
  mod(mgr->handle().id, EPOLL_CTL_MOD, mgr->mask());
  if (remove && mgr->mask() == operation::none)
    del(mgr->handle());
}

void multiplexer_impl::del(socket handle) {
  mod(handle.id, EPOLL_CTL_DEL, operation::none);
  managers_.erase(handle.id);
  if (shutting_down_ && managers_.empty())
    running_ = false;
}

multiplexer_impl::manager_map::iterator
multiplexer_impl::del(manager_map::iterator it) {
  auto fd = it->second->handle().id;
  mod(fd, EPOLL_CTL_DEL, operation::none);
  auto new_it = managers_.erase(it);
  if (shutting_down_ && managers_.empty())
    running_ = false;
  return new_it;
}

void multiplexer_impl::mod(int fd, int op, operation events) {
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

util::error multiplexer_impl::poll_once(bool blocking) {
  // Calculates the timeout value for the epoll_wait call
  auto timeout = [&]() -> int {
    if (!blocking)
      return 0; // nonblocking
    if (current_timeout_ == std::nullopt)
      return -1; // No timeout
    auto now = time_point_cast<milliseconds>(system_clock::now());
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

void multiplexer_impl::handle_events(event_span events) {
  auto handle_result = [&](socket_manager_ptr& mgr, event_result res,
                           operation op) -> bool {
    if (res == event_result::done) {
      disable(mgr, op, true);
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
        if (!handle_result(mgr, mgr->handle_read_event(), operation::read))
          continue;
      }
      // Handle possible write event
      if ((event.events & EPOLLOUT) == EPOLLOUT) {
        if (!handle_result(mgr, mgr->handle_write_event(), operation::write))
          continue;
      }
    }
  }
}

#elif defined(KQUEUE_MPX) // -- kqueue specific implementation -----------------

void multiplexer_impl::add(socket_manager_ptr mgr, operation initial) {
  LOG_TRACE();
  if (is_multiplexer_thread()) {
    LOG_DEBUG("Adding socket_manager with ", NET_ARG2("id", mgr->handle().id),
              " for ", NET_ARG(initial));
    // Set nonblocking on socket
    if (!nonblocking(mgr->handle(), true))
      handle_error(util::error(util::error_code::socket_operation_failed,
                               "Could not set nonblocking"));
    // Add the mgr to the pollset for both reading and writing and enable it for
    // the initial operations
    mod(mgr->handle().id, (EV_ADD | EV_DISABLE), operation::read_write);
    enable(mgr, initial);
    managers_.emplace(mgr->handle().id, mgr);
    // TODO: This should probably return an error instead of calling
    // handle_error
    if (auto err = mgr->init())
      handle_error(err);
  } else {
    LOG_DEBUG("Requesting to add socket_manager with ",
              NET_ARG2("id", mgr->handle().id), " for ", NET_ARG(initial));
    mgr->ref();
    write_to_pipe(pollset_updater::add_code, mgr.get(), initial);
  }
}

void multiplexer_impl::enable(socket_manager_ptr mgr, operation op) {
  LOG_TRACE();
  LOG_DEBUG("Enabling mgr with ", NET_ARG2("id", mgr->handle().id),
            " registered for ", NET_ARG2("mask", to_string(mgr->mask())),
            " for ", NET_ARG2("new_event", to_string(op)));
  if (!mgr->mask_add(op))
    return;
  mod(mgr->handle().id, EV_ENABLE, mgr->mask());
}

void multiplexer_impl::disable(socket_manager_ptr mgr, operation op,
                               bool remove) {
  LOG_TRACE();
  LOG_DEBUG("Disabling mgr with ", NET_ARG2("id", mgr->handle().id),
            " registered for ", NET_ARG2("mask", to_string(mgr->mask())),
            " for ", NET_ARG2("event", to_string(op)));
  if (!mgr->mask_del(op))
    return;
  mod(mgr->handle().id, EV_DISABLE, op);
  if (remove && mgr->mask() == operation::none)
    del(mgr->handle());
}

void multiplexer_impl::del(socket handle) {
  LOG_TRACE();
  LOG_DEBUG("Deleting mgr with ", NET_ARG2("id", handle.id));
  mod(handle.id, EV_DELETE, operation::read_write);
  managers_.erase(handle.id);
  if (shutting_down_ && managers_.empty())
    running_ = false;
}

multiplexer_impl::manager_map::iterator
multiplexer_impl::del(manager_map::iterator it) {
  LOG_TRACE();
  auto fd = it->second->handle().id;
  LOG_DEBUG("Deleting mgr with ", NET_ARG2("id", fd));
  mod(fd, EV_DELETE, operation::read_write);
  auto new_it = managers_.erase(it);
  if (shutting_down_ && managers_.empty())
    running_ = false;
  return new_it;
}

void multiplexer_impl::mod(int fd, int op, operation events) {
  LOG_TRACE();
  LOG_DEBUG("Modifying mgr with ", NET_ARG2("id", fd), ", ", NET_ARG(op),
            ", for ", NET_ARG2("events", to_string(events)));
  static constexpr const auto to_kevent_filter
    = [](net::operation op) -> int16_t {
    switch (op) {
      case net::operation::none:
        return 0;
      case net::operation::read:
        return EVFILT_READ;
      case net::operation::write:
        return EVFILT_WRITE;
      default:
        return 0;
    }
  };

  // kqueue may not add sockets for both reading and writing in a single call..
  if (events == operation::read_write) {
    mod(fd, op, operation::read);
    mod(fd, op, operation::write);
  } else {
    event_type event;
    EV_SET(&event, fd, to_kevent_filter(events), op, 0, 0, nullptr);
    update_cache_.push_back(event);
  }
}

util::error multiplexer_impl::poll_once(bool blocking) {
  using namespace std::chrono;
  LOG_TRACE();
  // Calculates the timeout value for the kqueue call
  auto calculate_timeout = [this, blocking]() -> timespec {
    if (!blocking || !current_timeout_)
      return {0, 0};
    const auto now = time_point_cast<milliseconds>(system_clock::now());
    const auto timeout_tp = time_point_cast<milliseconds>(*current_timeout_);
    const auto diff_ms = (timeout_tp - now).count();
    return {(diff_ms / 1000), ((diff_ms % 1000) * 1000000)};
  };
  const auto timeout = calculate_timeout();
  LOG_DEBUG("kevent ", NET_ARG(blocking), ", timeout={", timeout.tv_sec, "s,",
            timeout.tv_nsec, "ns}");
  const int num_events = kevent(mpx_fd_, update_cache_.data(),
                                static_cast<int>(update_cache_.size()),
                                pollset_.data(),
                                static_cast<int>(pollset_.size()),
                                (!current_timeout_ ? nullptr : &timeout));
  update_cache_.clear();
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

void multiplexer_impl::handle_events(event_span events) {
  LOG_TRACE();
  LOG_DEBUG("Handling ", events.size(), " I/O events");
  auto handle_result = [this](socket_manager_ptr& mgr, event_result res,
                              operation op) {
    LOG_DEBUG(NET_ARG2("res", to_string(res)));
    switch (res) {
      case event_result::done:
        disable(mgr, op, true);
        break;
      case event_result::error:
        del(mgr->handle());
        break;
      default:
        // nop
        break;
    }
  };

  for (const auto& event : events) {
    const auto handle = socket{static_cast<net::socket_id>(event.ident)};
    if (event.flags & EV_EOF) {
      LOG_ERROR("EV_EOF on ", NET_ARG2("handle", handle.id), ": ",
                util::last_error_as_string());
      del(handle);
      continue;
    } else {
      auto& mgr = managers_[handle.id];
      switch (event.filter) {
        case EVFILT_READ:
          LOG_DEBUG("Handling EVFILT_READ on manager with ",
                    NET_ARG2("id", handle.id));
          handle_result(mgr, mgr->handle_read_event(), operation::read);
          break;
        case EVFILT_WRITE:
          LOG_DEBUG("Handling EVFILT_WRITE on manager with ",
                    NET_ARG2("id", handle.id));
          handle_result(mgr, mgr->handle_write_event(), operation::write);
          break;
        default:
          LOG_WARNING("Event filter unknown");
          break;
      }
    }
  }
}

#endif

} // namespace net
