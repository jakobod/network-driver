/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 30.03.2021
 */

#include "net/multiplexer.hpp"

#include <chrono>
#include <cstring>
#include <iostream>
#include <utility>

#include "benchmark/result.hpp"
#include "net/acceptor.hpp"
#include "net/socket_manager.hpp"
#include "net/socket_sys_includes.hpp"

namespace net {

using std::chrono::operator""ms;

multiplexer::multiplexer(benchmark::result_ptr results)
  : results_(std::move(results)),
    epoll_fd_(invalid_socket_id),
    shutting_down_(false),
    running_(false) {
  // nop
}

multiplexer::~multiplexer() {
  close(pipe_writer_);
  ::close(epoll_fd_);
}

detail::error multiplexer::init(socket_manager_factory_ptr factory) {
  epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd_ < 0)
    return detail::error(detail::runtime_error, "creating epoll fd failed");
  // Create pollset updater
  auto pipe_res = make_pipe();
  if (auto err = std::get_if<detail::error>(&pipe_res))
    return *err;
  auto pipe_fds = std::get<pipe_socket_pair>(pipe_res);
  pipe_reader_ = pipe_fds.first;

  pipe_writer_ = pipe_fds.second;
  add(std::make_shared<pollset_updater>(pipe_reader_, this), operation::read);
  // Create Acceptor
  auto res = net::make_tcp_accept_socket(0);
  if (auto err = std::get_if<detail::error>(&res))
    return *err;
  auto accept_socket_pair
    = std::get<std::pair<net::tcp_accept_socket, uint16_t>>(res);
  auto accept_socket = accept_socket_pair.first;
  std::cerr << "acceptor is listening on port: " << accept_socket_pair.second
            << " socket_id = " << accept_socket.id << std::endl;
  add(std::make_shared<acceptor>(accept_socket, this, std::move(factory)),
      operation::read);
  return detail::none;
}

void multiplexer::start() {
  if (!running_) {
    running_ = true;
    mpx_thread_ = std::thread(&multiplexer::run, this);
    mpx_thread_id_ = mpx_thread_.get_id();
  }
}

void multiplexer::shutdown() {
  if (std::this_thread::get_id() == mpx_thread_id_) {
    std::cerr << "shutdown() called by multiplexer" << std::endl;
    // disable all managers for reading!
    for (auto it = managers_.begin(); it != managers_.end(); ++it) {
      auto& mgr = it->second;
      if (mgr->handle() != pipe_reader_) {
        disable(*mgr, operation::read, false);
        if (mgr->mask() == operation::none)
          it = del(it);
      }
    }
    shutting_down_ = true;
    running_ = false;
  } else if (!shutting_down_) {
    std::cerr << "shutdown() called by outstanding" << std::endl;
    std::byte code{pollset_updater::shutdown_code};
    auto res = write(pipe_writer_, detail::byte_span(&code, 1));
    if (res != 1) {
      std::cerr << "ERROR could not write to pipe: "
                << last_socket_error_as_string() << std::endl;
      abort(); // Can't be handled by shutting down, if shutdown fails.
    }
  }
}

void multiplexer::join() {
  if (mpx_thread_.joinable())
    mpx_thread_.join();
}

// -- Error handling -----------------------------------------------------------

void multiplexer::handle_error(detail::error err) {
  std::cerr << "ERROR: " << err << std::endl;
  shutdown();
}

// -- Interface functions ------------------------------------------------------

void multiplexer::add(socket_manager_ptr mgr, operation initial) {
  if (!mgr->mask_add(initial))
    return;
  mod(mgr->handle().id, EPOLL_CTL_ADD, mgr->mask());
  managers_.emplace(mgr->handle().id, std::move(mgr));
}

void multiplexer::enable(socket_manager& mgr, operation op) {
  if (!mgr.mask_add(op))
    return;
  mod(mgr.handle().id, EPOLL_CTL_MOD, mgr.mask());
}

void multiplexer::disable(socket_manager& mgr, operation op, bool remove) {
  if (!mgr.mask_del(op))
    return;
  mod(mgr.handle().id, EPOLL_CTL_MOD, mgr.mask());
  if (remove && mgr.mask() == operation::none)
    del(mgr.handle());
}

void multiplexer::del(socket handle) {
  mod(handle.id, EPOLL_CTL_DEL, operation::none);
  managers_.erase(handle.id);
  if (shutting_down_ && managers_.empty())
    running_ = false;
  results_->count_closed();
}

multiplexer::manager_map::iterator multiplexer::del(manager_map::iterator it) {
  auto fd = it->second->handle().id;
  mod(fd, EPOLL_CTL_DEL, operation::none);
  auto new_it = managers_.erase(it);
  if (shutting_down_ && managers_.empty())
    running_ = false;
  results_->count_closed();
  return new_it;
}

void multiplexer::mod(int fd, int op, operation events) {
  epoll_event event = {};
  event.events = static_cast<uint32_t>(events);
  event.data.fd = fd;
  if (epoll_ctl(epoll_fd_, op, fd, &event) < 0)
    handle_error(detail::error(detail::runtime_error,
                               "epoll_ctl: " + std::string(strerror(errno))));
}

void multiplexer::run() {
  using namespace std::chrono;
  auto block_until = steady_clock::now() + 1000ms;
  while (running_) {
    auto next_wakeup
      = duration_cast<milliseconds>(block_until - steady_clock::now());
    auto num_events = epoll_wait(epoll_fd_, pollset_.data(),
                                 static_cast<int>(pollset_.size()),
                                 next_wakeup.count());
    if (num_events < 0) {
      switch (errno) {
        case EINTR:
          continue;
        default:
          std::cerr << "epoll_wait failed: " << strerror(errno) << std::endl;
          running_ = false;
          break;
      }
    } else {
      for (int i = 0; i < num_events; ++i) {
        if (pollset_[i].events == (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
          std::cerr << "epoll_wait failed: socket = " << i << strerror(errno)
                    << std::endl;
          del(socket{pollset_[i].data.fd});
          continue;
        } else {
          if ((pollset_[i].events & operation::read) == operation::read) {
            results_->count_read_event();
            auto mgr = managers_[static_cast<int>(pollset_[i].data.fd)];
            if (!mgr->handle_read_event())
              disable(*mgr, operation::read);
          }
          if ((pollset_[i].events & operation::write) == operation::write) {
            results_->count_write_event();
            auto mgr = managers_[static_cast<int>(pollset_[i].data.fd)];
            if (!mgr->handle_write_event())
              disable(*mgr, operation::write);
          }
        }
      }
      if (block_until <= steady_clock::now()) {
        block_until += 1000ms;
        std::cerr << *results_ << std::endl;
        // for (const auto& p : managers_)
        //   std::cerr << *p.second << std::endl;
        // std::cerr << std::endl;
      }
    }
  }
  std::cerr << "multiplexer done" << std::endl;
}

} // namespace net
