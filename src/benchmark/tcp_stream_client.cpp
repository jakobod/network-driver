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
    written_(0),
    running_(false),
    mt_(std::move(mt)),
    dist_(0, 100'000'000) {
  to_write_ = dist_(mt_);
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
    auto num_events = epoll_wait(epoll_fd_, pollset_.data(),
                                 static_cast<int>(pollset_.size()), -1);
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
          std::cerr << "read_event" << std::endl;
          auto read_res = read();
          if (read_res == state::error)
            stop();
        } else if ((pollset_[i].events & operation::write)
                   == operation::write) {
          // std::cerr << "write_event" << std::endl;
          auto write_res = write();
          if (write_res == state::error)
            stop();
          else if (write_res == state::done)
            reconnect();
        }
      }
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
  std::cerr << "connect 1" << std::endl;
  auto connection_res = net::make_connected_tcp_stream_socket(ip_, port_);
  std::cerr << "connect 2" << std::endl;
  if (auto err = std::get_if<detail::error>(&connection_res))
    return *err;
  handle_ = std::get<net::tcp_stream_socket>(connection_res);
  std::cerr << "connect 3" << std::endl;
  add(handle_);
  std::cerr << "connect 4" << std::endl;
  return none;
}

detail::error tcp_stream_client::reconnect() {
  std::cerr << "reconnecting" << std::endl;
  del(handle_);
  std::cerr << "del" << std::endl;
  written_ = 0;
  to_write_ = dist_(mt_);
  std::cerr << "dist" << std::endl;
  return connect();
}

tcp_stream_client::state tcp_stream_client::read() {
  auto read_result = net::read(handle_, data_);
  if (read_result > 0) {
    received_ += read_result;
  } else if (read == 0) {
    std::cout << "server disconnected" << std::endl;
    return tcp_stream_client::state::disconnected;
  } else {
    if (!net::last_socket_error_is_temporary()) {
      std::cerr << "ERROR read failed: " << net::last_socket_error_as_string()
                << std::endl;
      return tcp_stream_client::state::error;
    }
  }
  return tcp_stream_client::state::go_on;
}

tcp_stream_client::state tcp_stream_client::write() {
  auto write_res = net::write(handle_, data_);
  if (write_res <= 0) {
    if (!net::last_socket_error_is_temporary()) {
      std::cerr << "ERROR write failed: " << net::last_socket_error_as_string()
                << std::endl;
      return tcp_stream_client::state::error;
    }
  }
  written_ += write_res;
  if (written_ >= to_write_)
    return tcp_stream_client::state::done;
  return tcp_stream_client::state::go_on;
}

// -- epoll management -------------------------------------------------------

void tcp_stream_client::add(net::socket handle) {
  mod(handle.id, EPOLL_CTL_ADD, operation::read_write);
}

void tcp_stream_client::del(net::socket handle) {
  mod(handle.id, EPOLL_CTL_DEL, operation::none);
  close(handle);
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
