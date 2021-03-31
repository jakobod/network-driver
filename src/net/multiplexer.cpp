/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 30.03.2021
 */

#include "net/multiplexer.hpp"

#include "net/acceptor.hpp"
#include "net/socket_manager.hpp"

namespace net {

multiplexer::multiplexer() : running_(true) {
  epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd_ < 0) {
    std::cerr << "creating epoll fd failed" << std::endl;
    abort();
  }
}

multiplexer::~multiplexer() {
  ::close(epoll_fd_);
}

void multiplexer::init() {
  // create an acceptor and add it to the pollset.
}

void multiplexer::start() {
  mpx_thread_ = std::thread(&multiplexer::run, this);
  mpx_thread_id_ = mpx_thread_.get_id();
}

void multiplexer::register_new_socket_manager(
  [[maybe_unused]] socket_manager_ptr mgr) {
  // managers_.insert(mgr->handle(), std::move(mgr));
  // mgr->register_reading();
}

void multiplexer::register_reading(socket sock) {
  add(sock.id, operation::read);
}

void multiplexer::register_writing(socket sock) {
  add(sock.id, operation::write);
}

void multiplexer::add(int fd, operation op) {
  auto& mgr = managers_[fd];
  const auto old = mgr->mask();
  auto mask = old | op;
  if (mask != old)
    mod(fd, EPOLL_CTL_MOD, mask);
  mgr->mask_add(op);
}

void multiplexer::del(int fd, operation op) {
  auto& mgr = managers_[fd];
  const auto old = mgr->mask();
  auto mask = old & ~op;
  if (mask != old)
    mod(fd, EPOLL_CTL_MOD, mask);
  mgr->mask_del(op);
}

void multiplexer::mod(int fd, int op, operation events) {
  epoll_event event = {};
  event.events = static_cast<uint32_t>(events);
  event.data.fd = fd;
  if (epoll_ctl(epoll_fd_, op, fd, &event) < 0) {
    std::cerr << "epoll_ctl: " << strerror(errno) << std::endl;
    abort();
  }
}

void multiplexer::run() {
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
        } else if ((pollset_[i].events & operation::read) == operation::read) {
          managers_[i]->handle_read_event();
        } else if ((pollset_[i].events & operation::write)
                   == operation::write) {
          managers_[i]->handle_write_event();
        }
      }
    }
  }
}

} // namespace net
