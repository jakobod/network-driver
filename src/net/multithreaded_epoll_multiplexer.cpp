/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/multithreaded_epoll_multiplexer.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <utility>

#include "net/acceptor.hpp"
#include "net/error.hpp"
#include "net/event_result.hpp"
#include "net/socket_manager.hpp"
#include "net/socket_sys_includes.hpp"

namespace net {

multithreaded_epoll_multiplexer::multithreaded_epoll_multiplexer(
  size_t num_threads) {
  mpx_threads_.resize(num_threads);
}

multithreaded_epoll_multiplexer::~multithreaded_epoll_multiplexer() {
  ::close(epoll_fd_);
}

error multithreaded_epoll_multiplexer::init(socket_manager_factory_ptr factory,
                                            uint16_t port) {
  epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd_ < 0)
    return error(runtime_error, "creating epoll fd failed");
  // Create pollset updater
  auto pipe_res = make_pipe();
  if (auto err = get_error(pipe_res))
    return *err;
  auto pipe_fds = std::get<pipe_socket_pair>(pipe_res);
  pipe_reader_ = pipe_fds.first;
  pipe_writer_ = pipe_fds.second;
  add(std::make_shared<pollset_updater>(pipe_reader_, this), operation::read);
  // Create Acceptor
  auto res = net::make_tcp_accept_socket(port);
  if (auto err = get_error(res))
    return *err;
  auto accept_socket_pair = std::get<net::acceptor_pair>(res);
  auto accept_socket = accept_socket_pair.first;
  port_ = accept_socket_pair.second;
  add(std::make_shared<acceptor>(accept_socket, this, std::move(factory)),
      operation::read);
  return none;
}

void multithreaded_epoll_multiplexer::start() {
  if (!running_) {
    for (auto& thread : mpx_threads_) {
      thread = std::thread(&multithreaded_epoll_multiplexer::run, this);
      mpx_thread_ids_.emplace(thread.get_id());
    }
    running_ = true;
  }
}

void multithreaded_epoll_multiplexer::shutdown() {
  if (mpx_thread_ids_.contains(std::this_thread::get_id())) {
    auto it = managers_.begin();
    while (it != managers_.end()) {
      auto& mgr = it->second;
      if (mgr->handle() != pipe_reader_) {
        disable(*mgr, operation::read, false);
        if ((mgr->mask() & operation::read_write) == operation::none)
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
      abort(); // Can't be handled by shutting down, if shutdown fails.
    }
  }
}

void multithreaded_epoll_multiplexer::join() {
  for (auto& thread : mpx_threads_) {
    if (thread.joinable())
      thread.join();
  }
}

bool multithreaded_epoll_multiplexer::running() {
  return running_;
}

void multithreaded_epoll_multiplexer::set_thread_id() {
  mpx_thread_ids_.emplace(std::this_thread::get_id());
}

// -- Error handling -----------------------------------------------------------

void multithreaded_epoll_multiplexer::handle_error(error err) {
  std::cerr << "ERROR: " << err << std::endl;
  shutdown();
}

// -- Interface functions ------------------------------------------------------

error multithreaded_epoll_multiplexer::poll_once(bool blocking) {
  using namespace std::chrono;
  auto timeout = [&]() -> int {
    if (!blocking)
      return 0; // nonblocking
    if (current_timeout_ == std::nullopt)
      return -1;
    auto now = time_point_cast<milliseconds>(std::chrono::system_clock::now());
    auto timeout_tp = time_point_cast<milliseconds>(*current_timeout_);
    return (timeout_tp - now).count();
  };
  auto to = timeout();
  pollset events;
  auto num_events = epoll_wait(epoll_fd_, events.data(),
                               static_cast<int>(events.size()), to);
  if (num_events < 0) {
    return (errno == EINTR) ? none : error{runtime_error, strerror(errno)};
  } else {
    handle_timeouts();
    for (int i = 0; i < num_events; ++i) {
      auto& event = events[i];
      if (event.events == (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
        std::cerr << "epoll_wait failed: socket = " << i << strerror(errno)
                  << std::endl;
        del(socket{event.data.fd});
        continue;
      } else {
        auto& mgr = managers_[static_cast<int>(event.data.fd)];
        auto handle_result = [&](event_result res) -> bool {
          if (res == event_result::done) {
            disable(*mgr, operation::read);
          } else if (res == event_result::error) {
            del(mgr->handle());
            return false;
          }
          return true;
        };
        if ((event.events & operation::read) == operation::read) {
          if (!handle_result(mgr->handle_read_event()))
            continue;
        }
        if ((event.events & operation::write) == operation::write) {
          if (!handle_result(mgr->handle_write_event()))
            continue;
        }
        rearm(mgr);
      }
    }
  }
  return none;
}

void multithreaded_epoll_multiplexer::add(socket_manager_ptr mgr,
                                          operation initial) {
  if (!mgr->mask_add(initial | operation::one_shot))
    return;
  mod(mgr->handle().id, EPOLL_CTL_ADD, mgr->mask());
  managers_.emplace(mgr->handle().id, std::move(mgr));
}

void multithreaded_epoll_multiplexer::enable(socket_manager& mgr,
                                             operation op) {
  if (!mgr.mask_add(op))
    return;
  mod(mgr.handle().id, EPOLL_CTL_MOD, mgr.mask());
}

void multithreaded_epoll_multiplexer::disable(socket_manager& mgr, operation op,
                                              bool remove) {
  if (!mgr.mask_del(op))
    return;
  mod(mgr.handle().id, EPOLL_CTL_MOD, mgr.mask());
  if (remove && (mgr.mask() & operation::read_write) == operation::none)
    del(mgr.handle());
}

void multithreaded_epoll_multiplexer::set_timeout(
  socket_manager& mgr, uint64_t timeout_id,
  std::chrono::system_clock::time_point when) {
  timeouts_.emplace(mgr.handle().id, when, timeout_id);
  auto next_timeout = (current_timeout_ != std::nullopt)
                        ? std::min(when, *current_timeout_)
                        : when;
  set_current_timeout(next_timeout);
}

void multithreaded_epoll_multiplexer::del(socket handle) {
  mod(handle.id, EPOLL_CTL_DEL, operation::none);
  managers_.erase(handle.id);
  if (shutting_down_ && managers_.empty())
    running_ = false;
}

multithreaded_epoll_multiplexer::manager_map::iterator
multithreaded_epoll_multiplexer::del(manager_map::iterator it) {
  auto fd = it->second->handle().id;
  mod(fd, EPOLL_CTL_DEL, operation::none);
  auto new_it = managers_.erase(it);
  if (shutting_down_ && managers_.empty())
    running_ = false;
  return new_it;
}

void multithreaded_epoll_multiplexer::rearm(socket_manager_ptr& mgr) {
  mod(mgr->handle().id, EPOLL_CTL_MOD, mgr->mask());
}

void multithreaded_epoll_multiplexer::mod(int fd, int op, operation events) {
  epoll_event event = {};
  event.events = static_cast<uint32_t>(events);
  event.data.fd = fd;
  if (epoll_ctl(epoll_fd_, op, fd, &event) < 0)
    handle_error(
      error(runtime_error, "epoll_ctl: " + std::string(strerror(errno))));
}

void multithreaded_epoll_multiplexer::handle_timeouts() {
  using namespace std::chrono;
  if (timeouts_.empty() || !handle_timeouts_lock_.try_lock())
    return;
  auto now = time_point_cast<milliseconds>(std::chrono::system_clock::now());
  auto it = timeouts_.begin();
  for (; it != timeouts_.end(); ++it) {
    auto when = time_point_cast<milliseconds>(it->when);
    if (when <= now) {
      managers_.at(it->hdl)->handle_timeout(it->id);
    } else {
      if (timeouts_.empty())
        set_current_timeout(std::nullopt);
      else
        set_current_timeout(it->when);
      break;
    }
  }
  timeouts_.erase(timeouts_.begin(), it);
  handle_timeouts_lock_.unlock();
}

void multithreaded_epoll_multiplexer::set_current_timeout(
  optional_timepoint when) {
  std::lock_guard<std::mutex> guard(current_timeout_lock_);
  current_timeout_ = when;
}

void multithreaded_epoll_multiplexer::run() {
  while (running_) {
    if (poll_once(true))
      running_ = false;
  }
}

error_or<multiplexer_ptr>
make_multithreaded_epoll_multiplexer(socket_manager_factory_ptr factory,
                                     uint16_t port, size_t num_threads) {
  auto mpx = std::make_shared<multithreaded_epoll_multiplexer>(num_threads);
  if (auto err = mpx->init(std::move(factory), port))
    return err;
  return mpx;
}

} // namespace net
