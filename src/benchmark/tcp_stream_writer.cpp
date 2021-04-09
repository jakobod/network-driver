/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 07.04.2021
 */

#include "benchmark/tcp_stream_writer.hpp"

#include <iostream>
#include <variant>

#include "detail/socket_guard.hpp"
#include "net/tcp_stream_socket.hpp"

namespace benchmark {

using detail::none;

tcp_stream_writer::tcp_stream_writer(const std::string ip, const uint16_t port,
                                     std::mt19937 mt)
  : ip_(std::move(ip)),
    port_(port),
    written_(0),
    running_(false),
    mt_(std::move(mt)),
    dist_(0, 100'000'000) {
  to_write_ = dist_(mt_);
}

tcp_stream_writer::~tcp_stream_writer() {
  close(handle_);
}

detail::error tcp_stream_writer::init() {
  return connect();
}

void tcp_stream_writer::run() {
  auto check_error = [&](tcp_stream_writer::state res) {
    switch (res) {
      case tcp_stream_writer::state::done:
        std::cout << "res = tcp_stream_writer::state::done" << std::endl;
        reconnect();
        break;
      case tcp_stream_writer::state::error:
        std::cout << "res = tcp_stream_writer::state::error" << std::endl;
        stop();
        break;
      default:
        break;
    }
  };
  while (running_) {
    auto write_res = write();
    check_error(read());
    check_error(write_res);
  }
}

void tcp_stream_writer::start() {
  running_ = true;
  writer_thread_ = std::thread(&tcp_stream_writer::run, this);
}

void tcp_stream_writer::stop() {
  running_ = false;
}

void tcp_stream_writer::join() {
  if (writer_thread_.joinable())
    writer_thread_.join();
}

detail::error tcp_stream_writer::connect() {
  auto connection_res = net::make_connected_tcp_stream_socket(ip_, port_);
  if (auto err = std::get_if<detail::error>(&connection_res))
    return *err;
  handle_ = std::get<net::tcp_stream_socket>(connection_res);
  return none;
}

detail::error tcp_stream_writer::reconnect() {
  close(handle_);
  written_ = 0;
  to_write_ = dist_(mt_);
  return connect();
}

tcp_stream_writer::state tcp_stream_writer::read() {
  auto read = net::read(handle_, data_);
  if (read <= 0) {
    if (!net::last_socket_error_is_temporary()) {
      std::cerr << "ERROR read failed: " << net::last_socket_error_as_string()
                << std::endl;
      return tcp_stream_writer::state::error;
    }
  }
  return tcp_stream_writer::state::go_on;
}

tcp_stream_writer::state tcp_stream_writer::write() {
  auto write_res = net::write(handle_, data_);
  if (write_res <= 0) {
    if (!net::last_socket_error_is_temporary()) {
      std::cerr << "ERROR write failed: " << net::last_socket_error_as_string()
                << std::endl;
      return tcp_stream_writer::state::error;
    }
  }
  written_ += write_res;
  if (written_ >= to_write_)
    return tcp_stream_writer::state::done;
  return tcp_stream_writer::state::go_on;
}

} // namespace benchmark
