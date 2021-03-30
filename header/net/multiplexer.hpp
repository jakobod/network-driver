/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 30.03.2021
 */

#pragma once

#include <array>
#include <cstring>
#include <functional>
#include <iostream>
#include <sys/epoll.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#include "net/fwd.hpp"
#include "net/operation.hpp"

namespace net {

static const size_t max_epoll_events = 32;

class multiplexer {
  using pollset = std::array<epoll_event, max_epoll_events>;
  using epoll_fd = int;
  using manager_map = std::unordered_map<int, socket_manager_ptr>;

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

  void register_new_manager(int fd) {
  }

  /// Registers `mgr` for read events.
  /// @thread-safe
  void register_reading(socket sock) {
    add(sock.id, operation::read);
  }

  /// Registers `mgr` for write events.
  /// @thread-safe
  void register_writing(socket sock) {
    add(sock.id, operation::write);
  }

  void add(int fd, operation op) {
    auto& mgr = managers_[fd];
    const auto old = mgr->mask();
    auto mask = old | op;
    if (mask != old)
      mod(fd, EPOLL_CTL_MOD, mask);
    mgr->mask_add(op);
  }

  void del(int fd, operation op) {
    auto& mgr = managers_[fd];
    const auto old = mgr->mask();
    auto mask = old & ~op;
    if (mask != old)
      mod(fd, EPOLL_CTL_MOD, mask);
    mgr->mask_del(op);
  }

private:
  void mod(int fd, int op, operation events) {
    epoll_event event = {};
    event.events = static_cast<uint32_t>(events);
    event.data.fd = fd;
    if (epoll_ctl(epoll_fd_, op, fd, &event) < 0) {
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
            // TODO: delete all references to handler!
          } else if ((pollset_[i].events & operation::read)
                     == operation::read) {
            managers_[i]->handle_read_event();
          } else if ((pollset_[i].events & operation::write)
                     == operation::write) {
            managers_[i]->handle_write_event();
          }
        }
      }
    }
  }

  epoll_fd epoll_fd_;
  pollset pollset_;
  manager_map managers_;

  bool running_;
  std::thread mpx_thread_;
  std::thread::id mpx_thread_id_;
};

} // namespace net
