/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#if defined(__linux__)

#  include "net/epoll_multiplexer.hpp"

#  include "net/acceptor.hpp"
#  include "net/operation.hpp"
#  include "net/socket_manager.hpp"
#  include "net/socket_sys_includes.hpp"

#  include "util/error.hpp"
#  include "util/format.hpp"

#  include <algorithm>
#  include <chrono>
#  include <iostream>
#  include <utility>

using std::chrono::milliseconds;
using std::chrono::system_clock;
using std::chrono::time_point_cast;

namespace {

uint32_t to_epoll_flag(net::operation op) {
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
}

} // namespace

namespace net {

epoll_multiplexer::epoll_multiplexer() : epoll_fd_(invalid_socket_id) {
  // nop
}

epoll_multiplexer::~epoll_multiplexer() {
  ::close(epoll_fd_);
}

util::error epoll_multiplexer::init(socket_manager_factory_ptr factory,
                                    uint16_t port) {
  epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd_ < 0)
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
  auto res = net::make_tcp_accept_socket(port);
  if (auto err = get_error(res))
    return *err;
  auto accept_socket_pair = std::get<net::acceptor_pair>(res);
  auto accept_socket = accept_socket_pair.first;
  port_ = accept_socket_pair.second;
  add(std::make_shared<acceptor>(accept_socket, this, std::move(factory)),
      operation::read);
  return util::none;
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
      exit(-1); // Can't be handled by shutting down, if shutdown fails.
    }
  }
}

void epoll_multiplexer::join() {
  if (mpx_thread_.joinable())
    mpx_thread_.join();
}

bool epoll_multiplexer::running() const {
  return mpx_thread_.joinable();
}

void epoll_multiplexer::set_thread_id() {
  mpx_thread_id_ = std::this_thread::get_id();
}

// -- Error handling -----------------------------------------------------------

void epoll_multiplexer::handle_error(const util::error& err) {
  std::cerr << "ERROR: " << err << std::endl;
  shutdown();
}

// -- Interface functions ------------------------------------------------------

void epoll_multiplexer::add(socket_manager_ptr mgr, operation initial) {
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

uint64_t
epoll_multiplexer::set_timeout(socket_manager& mgr,
                               std::chrono::system_clock::time_point when) {
  timeouts_.emplace(mgr.handle().id, when, current_timeout_id_);
  current_timeout_ = (current_timeout_ != std::nullopt)
                       ? std::min(when, *current_timeout_)
                       : when;
  return current_timeout_id_++;
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
  event.events = to_epoll_flag(events);
  event.data.fd = fd;
  if (epoll_ctl(epoll_fd_, op, fd, &event) < 0) {
    handle_error(
      {util::error_code::runtime_error,
       util::format("epoll_ctl: {0}", util::last_error_as_string())});
  }
}

util::error epoll_multiplexer::poll_once(bool blocking) {
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
  auto num_events = epoll_wait(epoll_fd_, pollset_.data(),
                               static_cast<int>(pollset_.size()), timeout());
  // Check for errors
  if (num_events < 0) {
    return (errno == EINTR) ? util::none
                            : util::error{util::error_code::runtime_error,
                                          util::last_error_as_string()};
  }
  // Handle all timeouts and io-events that have been registered
  handle_timeouts();
  handle_events(num_events);
  return util::none;
}

void epoll_multiplexer::handle_timeouts() {
  auto now = time_point_cast<milliseconds>(system_clock::now());
  for (auto it = timeouts_.begin(); it != timeouts_.end(); ++it) {
    auto& entry = *it;
    auto when = time_point_cast<milliseconds>(entry.when_);
    if (when <= now) {
      // Registered timeout has expired
      managers_.at(entry.handle_)->handle_timeout(entry.id_);
    } else {
      // Timeout not expired, delete handled entries and set the current timeout
      timeouts_.erase(timeouts_.begin(), it);
      if (timeouts_.empty())
        current_timeout_ = std::nullopt; // No timeout
      else
        current_timeout_ = entry.when_;
      break;
    }
  }
}

void epoll_multiplexer::handle_events(int num_events) {
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

  for (int i = 0; i < num_events; ++i) {
    auto& event = pollset_[i];
    if (event.events == (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
      std::cerr << util::format("epoll_wait failed: socket = {0}: {1}", i,
                                util::last_error_as_string())
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

void epoll_multiplexer::run() {
  while (running_) {
    if (poll_once(true))
      running_ = false;
  }
}

util::error_or<multiplexer_ptr>
make_epoll_multiplexer(socket_manager_factory_ptr factory, uint16_t port) {
  auto mpx = std::make_shared<epoll_multiplexer>();
  if (auto err = mpx->init(std::move(factory), port))
    return err;
  return mpx;
}

} // namespace net

#endif
