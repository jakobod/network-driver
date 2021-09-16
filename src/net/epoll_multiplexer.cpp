/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/epoll_multiplexer.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <utility>

#include "net/acceptor.hpp"
#include "net/error.hpp"
#include "net/socket_manager.hpp"
#include "net/socket_sys_includes.hpp"

namespace net {

epoll_multiplexer::epoll_multiplexer()
  : epoll_fd_(invalid_socket_id), shutting_down_(false), running_(false) {
  // nop
}

epoll_multiplexer::~epoll_multiplexer() {
  ::close(epoll_fd_);
}

error epoll_multiplexer::init(socket_manager_factory_ptr factory,
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

void epoll_multiplexer::start() {
  if (!running_) {
    running_ = true;
    mpx_thread_ = std::thread(&epoll_multiplexer::run, this);
    mpx_thread_id_ = mpx_thread_.get_id();
  }
}

void epoll_multiplexer::shutdown() {
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
      abort(); // Can't be handled by shutting down, if shutdown fails.
    }
  }
}

void epoll_multiplexer::join() {
  if (mpx_thread_.joinable())
    mpx_thread_.join();
}

bool epoll_multiplexer::running() {
  return mpx_thread_.joinable();
}

void epoll_multiplexer::set_thread_id() {
  mpx_thread_id_ = std::this_thread::get_id();
}

// -- Error handling -----------------------------------------------------------

void epoll_multiplexer::handle_error(error err) {
  std::cerr << "ERROR: " << err << std::endl;
  shutdown();
}

// -- Interface functions ------------------------------------------------------

error epoll_multiplexer::poll_once(bool blocking) {
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
  auto num_events = epoll_wait(epoll_fd_, pollset_.data(),
                               static_cast<int>(pollset_.size()), to);
  //  next_wakeup.count());
  if (num_events < 0) {
    switch (errno) {
      case EINTR:
        return none;
      default:
        return error{runtime_error, strerror(errno)};
    }
  } else {
    handle_timeouts();
    for (int i = 0; i < num_events; ++i) {
      if (pollset_[i].events == (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
        std::cerr << "epoll_wait failed: socket = " << i << strerror(errno)
                  << std::endl;
        del(socket{pollset_[i].data.fd});
        continue;
      } else {
        if ((pollset_[i].events & operation::read) == operation::read) {
          auto mgr = managers_[static_cast<int>(pollset_[i].data.fd)];
          if (!mgr->handle_read_event())
            disable(*mgr, operation::read);
        }
        if ((pollset_[i].events & operation::write) == operation::write) {
          auto mgr = managers_[static_cast<int>(pollset_[i].data.fd)];
          if (!mgr->handle_write_event())
            disable(*mgr, operation::write);
        }
      }
    }
  }
  return none;
}

void epoll_multiplexer::add(socket_manager_ptr mgr, operation initial) {
  if (!mgr->mask_add(initial))
    return;
  mod(mgr->handle().id, EPOLL_CTL_ADD, mgr->mask());
  managers_.emplace(mgr->handle().id, std::move(mgr));
}

void epoll_multiplexer::enable(socket_manager& mgr, operation op) {
  if (!mgr.mask_add(op))
    return;
  mod(mgr.handle().id, EPOLL_CTL_MOD, mgr.mask());
}

void epoll_multiplexer::disable(socket_manager& mgr, operation op,
                                bool remove) {
  if (!mgr.mask_del(op))
    return;
  mod(mgr.handle().id, EPOLL_CTL_MOD, mgr.mask());
  if (remove && mgr.mask() == operation::none)
    del(mgr.handle());
}

void epoll_multiplexer::set_timeout(
  socket_manager& mgr, uint64_t timeout_id,
  std::chrono::system_clock::time_point when) {
  timeouts_.emplace(mgr.handle().id, when, timeout_id);
  current_timeout_ = (current_timeout_ != std::nullopt)
                       ? std::min(when, *current_timeout_)
                       : when;
}

void epoll_multiplexer::del(socket handle) {
  mod(handle.id, EPOLL_CTL_DEL, operation::none);
  managers_.erase(handle.id);
  if (shutting_down_ && managers_.empty())
    running_ = false;
}

epoll_multiplexer::manager_map::iterator
epoll_multiplexer::del(manager_map::iterator it) {
  auto fd = it->second->handle().id;
  mod(fd, EPOLL_CTL_DEL, operation::none);
  auto new_it = managers_.erase(it);
  if (shutting_down_ && managers_.empty())
    running_ = false;
  return new_it;
}

void epoll_multiplexer::mod(int fd, int op, operation events) {
  epoll_event event = {};
  event.events = static_cast<uint32_t>(events);
  event.data.fd = fd;
  if (epoll_ctl(epoll_fd_, op, fd, &event) < 0)
    handle_error(
      error(runtime_error, "epoll_ctl: " + std::string(strerror(errno))));
}

void epoll_multiplexer::handle_timeouts() {
  using namespace std::chrono;
  auto now = time_point_cast<milliseconds>(std::chrono::system_clock::now());
  for (auto it = timeouts_.begin(); it != timeouts_.end(); ++it) {
    auto& entry = *it;
    auto when = time_point_cast<milliseconds>(entry.when);
    if (when <= now) {
      managers_.at(entry.hdl)->handle_timeout(entry.id);
    } else {
      timeouts_.erase(timeouts_.begin(), it);
      if (timeouts_.empty())
        current_timeout_ = std::nullopt;
      else
        current_timeout_ = entry.when;
      break;
    }
  }
}

void epoll_multiplexer::run() {
  while (running_) {
    if (poll_once(true))
      running_ = false;
  }
}

error_or<multiplexer_ptr>
make_epoll_multiplexer(socket_manager_factory_ptr factory, uint16_t port) {
  auto mpx = std::make_shared<epoll_multiplexer>();
  if (auto err = mpx->init(std::move(factory), port))
    return err;
  return mpx;
}

} // namespace net
