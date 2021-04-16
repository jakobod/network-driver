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
  for (int i = 0; i < 20; ++i) {
    auto read_res = read(handle<tcp_stream_socket>(), read_buf_);
    if (read_res == 0)
      return false;
    if (read_res < 0) {
      if (last_socket_error_is_temporary()) {
        return true;
      } else {
        mpx()->handle_error(detail::error(detail::socket_operation_failed,
                                          "socket_manager.read(): "
                                            + last_socket_error_as_string()));
        return false;
      }
    }
    // results_->add_received_bytes(res);
    received_ += read_res;
    write_buffer_.insert(write_buffer_.begin(), read_buf_.begin(),
                         read_buf_.begin() + read_res);
    register_writing();
  }
  return true;
}

bool socket_manager_impl::handle_write_event() {
  for (int i = 0; i < 20; ++i) {
    auto write_res = write(handle<tcp_stream_socket>(), write_buffer_);
    if (write_res > 0) {
      write_buffer_.erase(write_buffer_.begin(),
                          write_buffer_.begin() + write_res);
      // results_->add_sent_bytes(written);
      written_ += write_res;
      return !write_buffer_.empty();
    } else if (write_res <= 0) {
      if (last_socket_error_is_temporary())
        return true;
      else
        mpx()->handle_error(detail::error(detail::socket_operation_failed,
                                          "socket_manager.write(): "
                                            + last_socket_error_as_string()));
    }
  }
  return false;
}

} // namespace benchmark
