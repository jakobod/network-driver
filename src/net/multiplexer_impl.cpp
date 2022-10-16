/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/multiplexer_impl.hpp"

#include "net/acceptor.hpp"
#include "net/operation.hpp"
#include "net/pollset_updater.hpp"
#include "net/socket_manager.hpp"
#include "net/socket_sys_includes.hpp"

#include "util/error.hpp"
#include "util/format.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <span>
#include <utility>

// -- identical implementation of the multiplexer_impl accross OSes ------------

namespace net {

multiplexer_impl::~multiplexer_impl() {
  ::close(mpx_fd_);
}

util::error multiplexer_impl::init(socket_manager_factory_ptr factory,
                                   uint16_t port, bool local) {
#if defined(EPOLL_MPX)
  mpx_fd_ = epoll_create1(EPOLL_CLOEXEC);
#elif defined(KQUEUE_MPX)
  mpx_fd_ = kqueue();
#endif
  if (mpx_fd_ < 0)
    return {util::error_code::runtime_error, "creating epoll fd failed"};
  // Create pollset updater
  auto pipe_res = make_pipe();
  if (auto err = util::get_error(pipe_res))
    return *err;
  auto pipe_fds = std::get<pipe_socket_pair>(pipe_res);
  pipe_reader_ = pipe_fds.first;
  pipe_writer_ = pipe_fds.second;
  add(std::make_shared<pollset_updater>(pipe_reader_, this), operation::read);
  // Create Acceptor
  auto res = net::make_tcp_accept_socket(
    {(local ? ip::v4_address::localhost : ip::v4_address::any), port});
  if (auto err = get_error(res))
    return *err;
  auto accept_socket_pair = std::get<net::acceptor_pair>(res);
  auto accept_socket = accept_socket_pair.first;
  port_ = accept_socket_pair.second;
  add(std::make_shared<acceptor>(accept_socket, this, std::move(factory)),
      operation::read);
  return util::none;
}

void multiplexer_impl::start() {
  if (!running_) {
    running_ = true;
    mpx_thread_ = std::thread(&multiplexer_impl::run, this);
    mpx_thread_id_ = mpx_thread_.get_id();
  }
}

void multiplexer_impl::shutdown() {
  if (std::this_thread::get_id() == mpx_thread_id_) {
    auto it = managers_.begin();
    while (it != managers_.end()) {
      auto& mgr = it->second;
      if (mgr->handle() != pipe_reader_) {
        disable(*mgr, operation::read, false);
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
    std::byte code{pollset_updater::shutdown_code};
    auto res = write(pipe_writer_, util::byte_span(&code, 1));
    if (res != 1) {
      std::cerr << "ERROR could not write to pipe: "
                << last_socket_error_as_string() << std::endl;
      exit(-1); // Can't be handled by shutting down, if shutdown fails.
    }
  }
}

void multiplexer_impl::join() {
  if (mpx_thread_.joinable())
    mpx_thread_.join();
}

bool multiplexer_impl::running() const {
  return mpx_thread_.joinable();
}

void multiplexer_impl::set_thread_id() {
  mpx_thread_id_ = std::this_thread::get_id();
}

void multiplexer_impl::run() {
  while (running_) {
    if (poll_once(true))
      running_ = false;
  }
}

// -- Error handling -----------------------------------------------------------

void multiplexer_impl::handle_error(const util::error& err) {
  std::cerr << "ERROR: " << err << std::endl;
  shutdown();
}

// -- Timeout management -------------------------------------------------------

uint64_t multiplexer_impl::set_timeout(
  [[maybe_unused]] socket_manager& mgr,
  [[maybe_unused]] std::chrono::system_clock::time_point when) {
  // timeouts_.emplace(mgr.handle().id, when, current_timeout_id_);
  // current_timeout_ = (current_timeout_ != std::nullopt)
  //                      ? std::min(when, *current_timeout_)
  //                      : when;
  // return current_timeout_id_++;
  return 0;
}

void multiplexer_impl::handle_timeouts() {
  // auto now = time_point_cast<milliseconds>(system_clock::now());
  // for (auto it = timeouts_.begin(); it != timeouts_.end(); ++it) {
  //   auto& entry = *it;
  //   auto when = time_point_cast<milliseconds>(entry.when_);
  //   if (when <= now) {
  //     // Registered timeout has expired
  //     managers_.at(entry.handle_)->handle_timeout(entry.id_);
  //   } else {
  //     // Timeout not expired, delete handled entries and set the current
  //     timeout timeouts_.erase(timeouts_.begin(), it); if (timeouts_.empty())
  //       current_timeout_ = std::nullopt; // No timeout
  //     else
  //       current_timeout_ = entry.when_;
  //     break;
  //   }
  // }
}

util::error_or<multiplexer_ptr>
make_multiplexer(socket_manager_factory_ptr factory, uint16_t port) {
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

void multiplexer_impl::enable(socket_manager& mgr, operation op) {
  if (!mgr.mask_add(op))
    return;
  mod(mgr.handle().id, EPOLL_CTL_MOD, mgr.mask());
}

void multiplexer_impl::disable(socket_manager& mgr, operation op, bool remove) {
  if (!mgr.mask_del(op))
    return;
  mod(mgr.handle().id, EPOLL_CTL_MOD, mgr.mask());
  if (remove && mgr.mask() == operation::none)
    del(mgr.handle());
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
      case net::operation::none: return 0;
      case net::operation::read: return EPOLLIN;
      case net::operation::write: return EPOLLOUT;
      case net::operation::read_write: return (EPOLLIN | EPOLLOUT);
      default: return 0;
    }
  };
  epoll_event event{};
  event.events = to_epoll_flag(events);
  event.data.fd = fd;
  if (epoll_ctl(mpx_fd_, op, fd, &event) < 0) {
    handle_error(
      {util::error_code::runtime_error,
       util::format("epoll_ctl: {0}", util::last_error_as_string())});
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
  handle_events(
    std::span<event_type>(pollset_.data(), static_cast<size_t>(num_events)));
  return util::none;
}

void multiplexer_impl::handle_events(event_span events) {
  auto handle_result = [&](socket_manager_ptr& mgr, event_result res,
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
      std::cerr << util::format("epoll_wait failed: socket = {0}: {1}",
                                event.data.fd, util::last_error_as_string())
                << std::endl;
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
  // Set nonblocking behavior on socket
  if (!nonblocking(mgr->handle(), true))
    handle_error(util::error(util::error_code::socket_operation_failed,
                             "Could not set nonblocking"));
  // Add the mgr to the pollset for both reading and writing and enable it for
  // the initial operations
  mod(mgr->handle().id, (EV_ADD | EV_DISABLE), operation::read_write);
  enable(*mgr, initial);
  managers_.emplace(mgr->handle().id, mgr);
  // TODO: This should probably return an error instead of calling handle_error
  if (auto err = mgr->init())
    handle_error(err);
}

void multiplexer_impl::enable(socket_manager& mgr, operation op) {
  if (!mgr.mask_add(op))
    return;
  mod(mgr.handle().id, EV_ENABLE, mgr.mask());
}

void multiplexer_impl::disable(socket_manager& mgr, operation op, bool remove) {
  if (!mgr.mask_del(op))
    return;
  mod(mgr.handle().id, EV_DISABLE, op);
  if (remove && mgr.mask() == operation::none)
    del(mgr.handle());
}

void multiplexer_impl::del(socket handle) {
  const auto fd = handle.id;
  mod(fd, EV_DELETE, operation::read_write);
  managers_.erase(handle.id);
  if (shutting_down_ && managers_.empty())
    running_ = false;
}

multiplexer_impl::manager_map::iterator
multiplexer_impl::del(manager_map::iterator it) {
  auto fd = it->second->handle().id;
  mod(fd, EV_DELETE, operation::read_write);
  auto new_it = managers_.erase(it);
  if (shutting_down_ && managers_.empty())
    running_ = false;
  return new_it;
}

void multiplexer_impl::mod(int fd, int op, operation events) {
  static constexpr const auto to_kevent_filter
    = [](net::operation op) -> int16_t {
    switch (op) {
      case net::operation::none: return 0;
      case net::operation::read: return EVFILT_READ;
      case net::operation::write: return EVFILT_WRITE;
      default: return 0;
    }
  };

  // kqueue may not add sockets for both reading and writing in a single call..
  if (events == operation::read_write) {
    mod(fd, op, operation::read);
    mod(fd, op, operation::write);
  } else {
    event_type event;
    EV_SET(&event, fd, to_kevent_filter(events), op, 0, 0, nullptr);
    if (kevent(mpx_fd_, &event, 1, nullptr, 0, nullptr) < 0) {
      const util::error err{util::error_code::runtime_error,
                            util::format("kevent: {0}",
                                         util::last_error_as_string())};
      // ignore ENOENT for now.
      if (net::last_socket_error() != ENOENT)
        handle_error(err);
      else
        std::cerr << err << std::endl;
    }
  }
}

util::error multiplexer_impl::poll_once([[maybe_unused]] bool blocking) {
  // Poll for events on the reqistered sockets
  static constexpr const timespec nonblocking{0, 0};
  const int num_events = kevent(mpx_fd_, nullptr, 0, pollset_.data(),
                                static_cast<int>(pollset_.size()),
                                (blocking ? nullptr : &nonblocking));
  // Check for errors
  if (num_events < 0) {
    return (errno == EINTR) ? util::none
                            : util::error{util::error_code::runtime_error,
                                          util::last_error_as_string()};
  }
  // Handle all timeouts and io-events that have been registered
  handle_timeouts();
  handle_events(
    std::span<event_type>(pollset_.data(), static_cast<size_t>(num_events)));
  return util::none;
}

void multiplexer_impl::handle_events(event_span events) {
  auto handle_result = [this](socket_manager_ptr& mgr, event_result res,
                              operation op) {
    switch (res) {
      case event_result::done: disable(*mgr, op, true); break;
      case event_result::error: del(mgr->handle()); break;
      default:
        // nop
        break;
    }
  };

  for (const auto& event : events) {
    const auto handle = socket{static_cast<net::socket_id>(event.ident)};
    if (event.flags & EV_EOF) {
      std::cerr << util::format("epoll_wait failed: socket = {0}: {1}",
                                handle.id, util::last_error_as_string())
                << std::endl;
      del(handle);
      continue;
    } else {
      auto& mgr = managers_[handle.id];
      switch (event.filter) {
        case EVFILT_READ:
          handle_result(mgr, mgr->handle_read_event(), operation::read);
          break;
        case EVFILT_WRITE:
          handle_result(mgr, mgr->handle_write_event(), operation::write);
          break;
        default:
          // nop
          break;
      }
    }
  }
}

#endif

} // namespace net
