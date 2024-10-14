/**
 *  @author    Jakob Otto
 *  @file      kqueue_multiplexer.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/kqueue_multiplexer.hpp"

#include "net/acceptor.hpp"
#include "net/event_result.hpp"
#include "net/operation.hpp"
#include "net/pollset_updater.hpp"
#include "net/socket/pipe_socket.hpp"
#include "net/socket/tcp_accept_socket.hpp"
#include "net/socket_manager.hpp"

#include "util/binary_serializer.hpp"
#include "util/byte_span.hpp"
#include "util/config.hpp"
#include "util/error.hpp"
#include "util/error_or.hpp"
#include "util/format.hpp"
#include "util/intrusive_ptr.hpp"
#include "util/logger.hpp"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <memory>
#include <thread>
#include <unistd.h>
#include <utility>

namespace net {

kqueue_multiplexer::~kqueue_multiplexer() {
  LOG_TRACE();
  ::close(mpx_fd_);
}

util::error kqueue_multiplexer::init(socket_manager_factory_ptr factory,
                                     const util::config& cfg) {
  LOG_DEBUG("initializing kqueue multiplexer");
  LOG_TRACE();
  set_thread_id(std::this_thread::get_id());
  cfg_ = std::addressof(cfg);

  mpx_fd_ = kqueue();
  if (mpx_fd_ < 0) {
    return {util::error_code::runtime_error,
            "[kqueue_multiplexer]: Creating epoll fd failed"};
  }
  LOG_DEBUG("Created ", NET_ARG(mpx_fd_));

  // Create pollset updater
  const auto pipe_res = make_pipe();
  if (auto err = util::get_error(pipe_res)) {
    return *err;
  }
  auto pipe_fds = std::get<pipe_socket_pair>(pipe_res);
  pipe_reader_ = pipe_fds.first;
  pipe_writer_ = pipe_fds.second;
  add(util::make_intrusive<pollset_updater>(pipe_reader_, this),
      operation::read);
  // Create Acceptor
  const auto res = net::make_tcp_accept_socket(ip::v4_endpoint(
    (cfg_->get_or("multiplexer.local", true) ? ip::v4_address::localhost
                                             : ip::v4_address::any),
    cfg_->get_or<std::int64_t>("multiplexer.port", 0)));
  if (auto err = util::get_error(res)) {
    return *err;
  }
  const auto accept_socket_pair = std::get<net::acceptor_pair>(res);
  const auto accept_socket = accept_socket_pair.first;
  port_ = accept_socket_pair.second;
  LOG_DEBUG("listening on ", NET_ARG2("port", port_));
  add(util::make_intrusive<acceptor>(accept_socket, this, std::move(factory)),
      operation::read);
  set_thread_id();
  return util::none;
}

void kqueue_multiplexer::start() {
  LOG_TRACE();
  if (!running_) {
    running_ = true;
    mpx_thread_ = std::thread(&kqueue_multiplexer::run, this);
    mpx_thread_id_ = mpx_thread_.get_id();
    LOG_DEBUG(NET_ARG(mpx_thread_id_));
  }
}

void kqueue_multiplexer::shutdown() {
  LOG_TRACE();
  if (is_multiplexer_thread()) {
    LOG_DEBUG("multiplexer shutting down");
    auto it = managers_.begin();
    while (it != managers_.end()) {
      auto& mgr = it->second;
      if (mgr->handle() != pipe_reader_) {
        disable(mgr, operation::read, false);
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

void kqueue_multiplexer::join() {
  LOG_TRACE();
  if (mpx_thread_.joinable()) {
    LOG_DEBUG("joining on multiplexer thread");
    mpx_thread_.join();
  }
}

bool kqueue_multiplexer::running() const {
  return mpx_thread_.joinable();
}

void kqueue_multiplexer::set_thread_id(std::thread::id tid) noexcept {
  mpx_thread_id_ = tid;
}

void kqueue_multiplexer::run() {
  LOG_TRACE();
  while (running_) {
    if (poll_once(true)) {
      running_ = false;
    }
  }
}

// -- Error handling -----------------------------------------------------------

void kqueue_multiplexer::handle_error([[maybe_unused]] const util::error& err) {
  LOG_ERROR(err);
  shutdown();
}

// -- Timeout management -------------------------------------------------------

uint64_t
kqueue_multiplexer::set_timeout(socket_manager_ptr mgr,
                                std::chrono::system_clock::time_point when) {
  LOG_TRACE();
  LOG_DEBUG("Setting timeout ", current_timeout_id_, " on ",
            NET_ARG2("mgr", mgr->handle().id));
  timeouts_.emplace(mgr->handle().id, when, current_timeout_id_);
  current_timeout_
    = current_timeout_.has_value() ? std::min(when, *current_timeout_) : when;
  return current_timeout_id_++;
}

void kqueue_multiplexer::handle_timeouts() {
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

void kqueue_multiplexer::add(socket_manager_ptr mgr, operation initial) {
  LOG_TRACE();
  if (is_multiplexer_thread()) {
    LOG_DEBUG("Adding socket_manager with ", NET_ARG2("id", mgr->handle().id),
              " for ", NET_ARG(initial));
    // Set nonblocking on socket
    if (!nonblocking(mgr->handle(), true)) {
      handle_error(util::error(util::error_code::socket_operation_failed,
                               "Could not set nonblocking"));
    }
    // Add the mgr to the pollset for both reading and writing and enable it for
    // the initial operations
    mod(mgr->handle().id, (EV_ADD | EV_DISABLE), operation::read_write);
    enable(mgr, initial);
    managers_.emplace(mgr->handle().id, mgr);
    // TODO: This should probably return an error instead of calling
    // handle_error
    if (auto err = mgr->init(*cfg_)) {
      handle_error(err);
    }
  } else {
    LOG_DEBUG("Requesting to add socket_manager with ",
              NET_ARG2("id", mgr->handle().id), " for ", NET_ARG(initial));
    mgr->ref();
    write_to_pipe(pollset_updater::add_code, mgr.get(), initial);
  }
}

void kqueue_multiplexer::enable(socket_manager_ptr mgr, operation op) {
  LOG_TRACE();
  LOG_DEBUG("Enabling mgr with ", NET_ARG2("id", mgr->handle().id),
            " registered for ", NET_ARG2("mask", to_string(mgr->mask())),
            " for ", NET_ARG2("new_event", to_string(op)));
  if (!mgr->mask_add(op)) {
    return;
  }
  mod(mgr->handle().id, EV_ENABLE, mgr->mask());
}

void kqueue_multiplexer::disable(socket_manager_ptr mgr, operation op,
                                 bool remove) {
  LOG_TRACE();
  LOG_DEBUG("Disabling mgr with ", NET_ARG2("id", mgr->handle().id),
            " registered for ", NET_ARG2("mask", to_string(mgr->mask())),
            " for ", NET_ARG2("event", to_string(op)));
  if (!mgr->mask_del(op)) {
    return;
  }
  mod(mgr->handle().id, EV_DISABLE, op);
  if (remove && mgr->mask() == operation::none) {
    del(mgr->handle());
  }
}

void kqueue_multiplexer::del(socket handle) {
  LOG_TRACE();
  LOG_DEBUG("Deleting mgr with ", NET_ARG2("id", handle.id));
  mod(handle.id, EV_DELETE, operation::read_write);
  managers_.erase(handle.id);
  if (shutting_down_ && managers_.empty()) {
    running_ = false;
  }
}

kqueue_multiplexer::manager_map::iterator
kqueue_multiplexer::del(manager_map::iterator it) {
  LOG_TRACE();
  auto fd = it->second->handle().id;
  LOG_DEBUG("Deleting mgr with ", NET_ARG2("id", fd));
  mod(fd, EV_DELETE, operation::read_write);
  auto new_it = managers_.erase(it);
  if (shutting_down_ && managers_.empty()) {
    running_ = false;
  }
  return new_it;
}

void kqueue_multiplexer::mod(int fd, int op, operation events) {
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

util::error kqueue_multiplexer::poll_once(bool blocking) {
  using namespace std::chrono;
  LOG_TRACE();
  // Calculates the timeout value for the kqueue call
  auto calculate_timeout = [this, blocking]() -> timespec {
    if (!blocking || !current_timeout_) {
      return {0, 0};
    }
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

void kqueue_multiplexer::handle_events(event_span events) {
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

util::error_or<multiplexer_ptr>
make_kqueue_multiplexer(socket_manager_factory_ptr factory,
                        const util::config& cfg) {
  LOG_TRACE();
  auto mpx = std::make_shared<kqueue_multiplexer>();
  if (auto err = mpx->init(std::move(factory), cfg)) {
    return err;
  }
  return mpx;
}

} // namespace net
