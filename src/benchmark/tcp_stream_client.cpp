/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 07.04.2021
 */

#include "benchmark/tcp_stream_client.hpp"

#include <iostream>
#include <variant>

#include "detail/socket_guard.hpp"
#include "net/operation.hpp"
#include "net/tcp_stream_socket.hpp"

namespace benchmark {

using detail::none;
using net::operation;

tcp_stream_client::tcp_stream_client(const std::string ip, const uint16_t port,
                                     std::mt19937 mt)
  : ip_(std::move(ip)),
    port_(port),
    epoll_fd_(net::invalid_socket_id),
    write_goal_(0),
    written_(0),
    received_(0),
    event_mask_(operation::none),
    running_(false),
    mt_(std::move(mt)),
    dist_(0, 100'000'000) {
  write_goal_ = dist_(mt_);
}

tcp_stream_client::~tcp_stream_client() {
  close(handle_);
}

detail::error tcp_stream_client::init() {
  epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd_ < 0)
    return detail::error(detail::runtime_error, "creating epoll fd failed");
  return connect();
}

// -- thread functions -------------------------------------------------------

void tcp_stream_client::run() {
  while (running_) {
    std::cerr << "waiting on epoll_wait --------------------------------------"
              << std::endl;
    auto num_events = epoll_wait(epoll_fd_, pollset_.data(),
                                 static_cast<int>(pollset_.size()), -1);
    std::cerr << "epoll_wait returned -------------------------------------"
              << std::endl;
    if (num_events < 0) {
      switch (errno) {
        case EINTR:
          continue;
        default:
          std::cerr << "epoll_wait failed: "
                    << net::last_socket_error_as_string() << std::endl;
          running_ = false;
          break;
      }
    } else {
      for (int i = 0; i < num_events; ++i) {
        if (pollset_[i].events == (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
          std::cerr << "epoll_wait failed: socket = " << i
                    << net::last_socket_error_as_string() << std::endl;
          stop();
        } else if ((pollset_[i].events & operation::read) == operation::read) {
          auto read_res = read();
          if (read_res == state::error)
            stop();
          if (read_res == state::done) {
            std::cout << "done reading" << std::endl;
            disable(handle_, operation::read);
          }
        } else if ((pollset_[i].events & operation::write)
                   == operation::write) {
          auto write_res = write();
          if (write_res == state::error)
            stop();
          if (write_res == state::done) {
            std::cout << "done writing" << std::endl;
            disable(handle_, operation::write);
          }
        }
      }
      std::cerr << "wrote " << written_ << " bytes | received " << received_
                << " bytes" << std::endl;
    }
  }
  std::cerr << "tcp_stream_client done" << std::endl;
}

void tcp_stream_client::start() {
  running_ = true;
  writer_thread_ = std::thread(&tcp_stream_client::run, this);
}

void tcp_stream_client::stop() {
  running_ = false;
}

void tcp_stream_client::join() {
  if (writer_thread_.joinable())
    writer_thread_.join();
}

// -- Private member functions -----------------------------------------------

detail::error tcp_stream_client::connect() {
  std::cerr << "connect()" << std::endl;
  auto connection_res = net::make_connected_tcp_stream_socket(ip_, port_);
  if (auto err = std::get_if<detail::error>(&connection_res))
    return *err;
  handle_ = std::get<net::tcp_stream_socket>(connection_res);
  add(handle_);
  return none;
}

void tcp_stream_client::disconnect() {
  std::cerr << "disconnect()" << std::endl;
  del(handle_);
  shutdown(handle_, SHUT_RDWR);
  close(handle_);
}

detail::error tcp_stream_client::reconnect() {
  std::cerr << "reconnect()" << std::endl;
  disconnect();
  written_ = 0;
  write_goal_ = dist_(mt_);
  return connect();
}

tcp_stream_client::state tcp_stream_client::read() {
  auto read_result = net::read(handle_, data_);
  if (read_result > 0) {
    received_ += read_result;
    if (received_ >= write_goal_)
      return state::done;
  } else if (read_result == 0) {
    std::cout << "server disconnected" << std::endl;
    return state::disconnected;
  } else {
    if (!net::last_socket_error_is_temporary()) {
      std::cerr << "ERROR read failed: " << net::last_socket_error_as_string()
                << std::endl;
      return state::error;
    }
  }
  return state::go_on;
}

tcp_stream_client::state tcp_stream_client::write() {
  auto write_res = net::write(handle_, data_);
  if (write_res >= 0) {
    written_ += write_res;
    if (written_ >= write_goal_)
      return state::done;
  } else if (write_res < 0) {
    if (!net::last_socket_error_is_temporary()) {
      std::cerr << "ERROR write failed: " << net::last_socket_error_as_string()
                << std::endl;
      return state::error;
    }
  }
  return state::go_on;
}

// -- epoll management -------------------------------------------------------

bool tcp_stream_client::mask_add(operation flag) {
  if ((event_mask_ & flag) == flag)
    return false;
  event_mask_ = event_mask_ | flag;
  return true;
}

bool tcp_stream_client::mask_del(operation flag) {
  if ((event_mask_ & flag) == operation::none)
    return false;
  event_mask_ = event_mask_ & ~flag;
  return true;
}

void tcp_stream_client::add(net::socket handle) {
  nonblocking(handle, true);
  event_mask_ = operation::read_write;
  mod(handle.id, EPOLL_CTL_ADD, event_mask_);
}

void tcp_stream_client::del(net::socket handle) {
  event_mask_ = operation::none;
  mod(handle.id, EPOLL_CTL_DEL, event_mask_);
}

void tcp_stream_client::enable(net::socket handle, operation op) {
  if (!mask_add(op))
    return;
  mod(handle.id, EPOLL_CTL_MOD, event_mask_);
}

void tcp_stream_client::disable(net::socket handle, operation op) {
  if (!mask_del(op))
    return;
  mod(handle.id, EPOLL_CTL_MOD, event_mask_);
  if (event_mask_ == operation::none)
    reconnect();
}

void tcp_stream_client::mod(int fd, int op, operation events) {
  epoll_event event = {};
  event.events = static_cast<uint32_t>(events);
  event.data.fd = fd;
  if (epoll_ctl(epoll_fd_, op, fd, &event) < 0) {
    std::cerr << "ERROR epoll_ctl: " << net::last_socket_error_as_string()
              << std::endl;
    stop();
  }
}

} // namespace benchmark
