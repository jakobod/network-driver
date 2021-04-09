/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 30.03.2021
 */

#include "benchmark/socket_manager_impl.hpp"

#include <iostream>
#include <memory>

#include "net/tcp_stream_socket.hpp"

namespace benchmark {

using namespace net;

socket_manager_impl::socket_manager_impl(net::socket handle, multiplexer* mpx,
                                         result_ptr results)
  : socket_manager(handle, mpx), results_(std::move(results)) {
  // nop
}

socket_manager_impl::~socket_manager_impl() {
  // nop
}

bool socket_manager_impl::handle_read_event() {
  static constexpr int max_reads = 20;
  detail::byte_array<2048> buf;
  for (int i = 0; i < max_reads; ++i) {
    auto res = read(handle<tcp_stream_socket>(), buf);
    if (res == 0)
      return false;
    if (res < 0) {
      if (last_socket_error_is_temporary()) {
        return true;
      } else {
        mpx()->handle_error(detail::error(detail::socket_operation_failed,
                                          "socket_manager.read(): "
                                            + last_socket_error_as_string()));
        return false;
      }
    }
    results_->add_received_bytes(res);
    write_buffer_.insert(write_buffer_.begin(), buf.begin(), buf.begin() + res);
    register_writing();
  }
  return true;
}

bool socket_manager_impl::handle_write_event() {
  static constexpr int max_writes = 20;
  for (int i = 0; i < max_writes; ++i) {
    auto written = write(handle<tcp_stream_socket>(), write_buffer_);
    if (written > 0) {
      write_buffer_.erase(write_buffer_.begin(),
                          write_buffer_.begin() + written);
      results_->add_sent_bytes(written);
      return !write_buffer_.empty();
    } else if (written < 0) {
      if (last_socket_error_is_temporary())
        return true;
      else if (last_socket_error() == EPIPE)
        return false;
      else
        mpx()->handle_error(detail::error(detail::socket_operation_failed,
                                          "socket_manager.write(): "
                                            + last_socket_error_as_string()));
    }
  }
  return false;
}

} // namespace benchmark
