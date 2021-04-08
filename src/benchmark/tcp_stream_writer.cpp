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

tcp_stream_writer::tcp_stream_writer(size_t to_write)
  : to_write_(to_write), running_(false) {
  // nop
}

tcp_stream_writer::~tcp_stream_writer() {
  close(handle_);
}

detail::error tcp_stream_writer::init(const std::string ip,
                                      const uint16_t port) {
  auto connection_res = net::make_connected_tcp_stream_socket(ip, port);
  if (auto err = std::get_if<detail::error>(&connection_res))
    return *err;
  handle_ = std::get<net::tcp_stream_socket>(connection_res);
  std::cout << "done initializing the writer" << std::endl;
  return none;
}

void tcp_stream_writer::run() {
  while (running_) {
    if (!write() || !read())
      running_ = false;
  }
  std::cerr << "writer DONE!" << std::endl;
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

bool tcp_stream_writer::read() {
  auto read = net::read(handle_, data_);
  if (read <= 0) {
    if (!net::last_socket_error_is_temporary()) {
      std::cerr << "ERROR read failed: " << net::last_socket_error_as_string()
                << std::endl;
      return false;
    }
  }
  return true;
}

bool tcp_stream_writer::write() {
  auto written = net::write(handle_, data_);
  if (written <= 0) {
    if (!net::last_socket_error_is_temporary()) {
      std::cerr << "ERROR write failed: " << net::last_socket_error_as_string()
                << std::endl;
      return false;
    }
  }
  return true;
}

} // namespace benchmark
