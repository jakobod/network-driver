/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 02.03.2021
 */

#pragma once

#include <array>
#include <cstring>
#include <iostream>
#include <sys/epoll.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#include "net/fwd.hpp"
#include "net/operation.hpp"
#include "net/pipe_socket.hpp"

namespace net {

static const size_t max_epoll_events = 32;

class multiplexer {
  using pollset = std::array<epoll_event, max_epoll_events>;
  using epoll_fd = int;
  using event_mask = uint32_t;
  using event_map = std::unordered_map<int, event_mask>;
  using callback_map = std::unordered_map<int, socket_manager_ptr>;

public:
  multiplexer() : running_(true) {
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ < 0)
      std::cerr << "creating epoll fd failed" << std::endl;
  }

  ~multiplexer() {
    ::close(epoll_fd_);
  }

  void start() {
    mpx_thread_ = std::thread(&multiplexer::run, this);
    mpx_thread_id_ = mpx_thread_.get_id();
  }

  /// Registers `mgr` for read events.
  /// @thread-safe
  void register_reading(const socket_manager_ptr& mgr);

  /// Registers `mgr` for write events.
  /// @thread-safe
  void register_writing(const socket_manager_ptr& mgr);

  void add_fd(int fd, uint32_t initial) {
    mod(fd, EPOLL_CTL_ADD, initial);
    event_masks_[fd] = initial;
  }

  void add(int fd, operation op) {
    const auto old = event_masks_[fd];
    auto mask = old | op;
    if (mask != old)
      mod(fd, EPOLL_CTL_MOD, mask);
    event_masks_[fd] = mask;
  }

  void del(int fd, operation op) {
    const auto old = event_masks_[fd];
    auto mask = old & ~op;
    if (mask != old)
      mod(fd, EPOLL_CTL_MOD, mask);
    event_masks_[fd] = mask;
  }

private:
  void mod(int fd, int operation, uint32_t events) {
    epoll_event event = {};
    event.events = events;
    event.data.fd = fd;
    if (epoll_ctl(epoll_fd_, operation, fd, &event) < 0) {
      std::cerr << "epoll_ctl: " << strerror(errno) << std::endl;
      abort();
    }
  }

  void run() {
    while (running_) {
      auto num_events = epoll_wait(epoll_fd_, pollset_.data(),
                                   static_cast<int>(pollset_.size()), -1);
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
            del(i, operation::read_write);
          } else if (pollset_[i].events & operation::read) {
            // incoming data
          } else if (pollset_[i].events & operation::write) {
            // socket can write!
          }
        }
      }
    }
  }

  pipe_socket write_socket_;
  epoll_fd epoll_fd_;
  pollset pollset_;
  event_map event_masks_;
  callback_map callbacks_;

  std::thread mpx_thread_;
  bool running_;
  std::thread::id mpx_thread_id_;
};

} // namespace net
