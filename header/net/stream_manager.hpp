/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/socket_manager.hpp"

#include <utility>

#include "fwd.hpp"
#include "net/stream_socket.hpp"

namespace net {

/// Manages the lifetime of a socket.
template <class Application>
class stream_manager : public socket_manager {
public:
  template <class... Ts>
  stream_manager(stream_socket handle, multiplexer* mpx, Ts&&... xs)
    : socket_manager(handle, mpx), application_(std::forward<Ts>(xs)...) {
    // nop
  }

  detail::error init() {
    if (!net::nonblocking(handle(), true))
      return detail::error(detail::error_code::socket_operation_failed,
                           "Could not set nonblocking");
    return application_.init(*this);
  }

  // -- socket_manager API -----------------------------------------------------

  bool handle_read_event() override {
    for (int i = 0; i < 20; ++i) {
      auto ptr = recv_buffer_.data() + received_;
      auto size = recv_buffer_.size() - received_;
      auto read_res = read(handle<stream_socket>(), std::span(ptr, size));
      if (read_res == 0)
        return false; // Disconnect
      if (read_res < 0) {
        if (last_socket_error_is_temporary()) {
          return true;
        } else {
          mpx()->handle_error(detail::error(detail::socket_operation_failed,
                                            "[socket_manager.read()] "
                                              + last_socket_error_as_string()));
          return false;
        }
      }
      received_ += read_res;
      if (recv_buffer_.size() == received_)
        if (!application_.consume(*this, recv_buffer_))
          return false;
    }
    return true;
  }

  bool handle_write_event() override {
    for (int i = 0; i < 20; ++i) {
      auto num_bytes = send_buffer_.size() - written_;
      auto write_res = write(handle<stream_socket>(),
                             std::span(send_buffer_.data(), num_bytes));
      if (write_res > 0) {
        send_buffer_.erase(send_buffer_.begin(),
                           send_buffer_.begin() + write_res);

        return !send_buffer_.empty();
      } else if (write_res <= 0) {
        if (last_socket_error_is_temporary())
          return true;
        else
          mpx()->handle_error(detail::error(detail::socket_operation_failed,
                                            "[socket_manager.write()] "
                                              + last_socket_error_as_string()));
      }
    }
    return false;
  }

  std::string me() const override {
    return "stream_manager";
  }

  // -- public API -------------------------------------------------------------

  /// Configures the amount to be read next
  void configure_next_read(size_t size) {
    received_ = 0;
    if (recv_buffer_.size() != size)
      recv_buffer_.resize(size);
  }

  /// Returns a reference to the send_buffer
  detail::byte_buffer& send_buffer() {
    return send_buffer_;
  }

private:
  Application application_;

  size_t received_ = 0;
  size_t written_ = 0;

  detail::byte_buffer recv_buffer_;
  detail::byte_buffer send_buffer_;
};

} // namespace net
