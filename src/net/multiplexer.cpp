/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 30.03.2021
 */

#include "net/multiplexer.hpp"

#include <cstring>
#include <iostream>
#include <utility>

#include "net/acceptor.hpp"
#include "net/socket_manager.hpp"
#include "net/socket_sys_includes.hpp"

namespace net {

multiplexer::multiplexer() : epoll_fd_(invalid_socket_id), running_(false) {
  // nop
}

multiplexer::~multiplexer() {
  ::close(epoll_fd_);
}

detail::error multiplexer::init(socket_manager_factory_ptr factory) {
  epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd_ < 0)
    return detail::error(detail::runtime_error, "creating epoll fd failed");
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
  running_ = true;
  mpx_thread_ = std::thread(&multiplexer::run, this);
  mpx_thread_id_ = mpx_thread_.get_id();
}

// -- Error handling -----------------------------------------------------------

void multiplexer::handle_error(detail::error err) {
  // TODO: This should actually do a complete shutdown of the multiplexer!
  std::cerr << err << std::endl;
  abort();
}

// -- Interface functions ------------------------------------------------------

void multiplexer::add(socket_manager_ptr mgr, operation initial) {
  if (!mgr->mask_add(initial))
    return;
  std::cerr << "ADD op = " << to_string(initial)
            << " on fd = " << mgr->handle().id << std::endl;
  mod(mgr->handle().id, EPOLL_CTL_ADD, mgr->mask());
  managers_.emplace(std::make_pair(mgr->handle().id, std::move(mgr)));
}

void multiplexer::enable(socket_manager& mgr, operation op) {
  if (!mgr.mask_add(op))
    return;
  std::cerr << "ENABLE op = " << to_string(op) << " on fd = " << mgr.handle().id
            << std::endl;
  mod(mgr.handle().id, EPOLL_CTL_MOD, mgr.mask());
}

void multiplexer::disable(socket_manager& mgr, operation op) {
  if (!mgr.mask_del(op))
    return;
  std::cerr << "DISABLE op = " << to_string(op)
            << " on fd = " << mgr.handle().id << std::endl;
  mod(mgr.handle().id, EPOLL_CTL_MOD, mgr.mask());
  if (mgr.mask() == operation::none)
    del(mgr.handle());
}

void multiplexer::del(socket handle) {
  mod(handle.id, EPOLL_CTL_DEL, operation::none);
  managers_.erase(handle.id);
  std::cerr << "deleted fd = " << handle.id << std::endl;
}

void multiplexer::mod(int fd, int op, operation events) {
  std::cout << "mod: fd = " << fd << " op = " << op << std::endl;
  epoll_event event = {};
  event.events = static_cast<uint32_t>(events);
  event.data.fd = fd;
  if (epoll_ctl(epoll_fd_, op, fd, &event) < 0)
    handle_error(detail::error(detail::runtime_error,
                               "epoll_ctl: " + std::string(strerror(errno))));
}

void multiplexer::run() {
  while (running_) {
    std::cout << "waiting on epoll_wait" << std::endl;
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
          del(socket{pollset_[i].data.fd});
          // TODO: delete all references to handler!
        } else if ((pollset_[i].events & operation::read) == operation::read) {
          auto mgr = managers_[static_cast<int>(pollset_[i].data.fd)];
          if (!mgr->handle_read_event())
            disable(*mgr, operation::read);
        } else if ((pollset_[i].events & operation::write)
                   == operation::write) {
          auto mgr = managers_[static_cast<int>(pollset_[i].data.fd)];
          if (!mgr->handle_write_event())
            disable(*mgr, operation::write);
        }
      }
    }
  }
}

} // namespace net
