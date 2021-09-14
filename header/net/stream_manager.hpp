/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/socket_manager.hpp"

#include <utility>

#include "fwd.hpp"
#include "net/error.hpp"
#include "net/stream_socket.hpp"

namespace net {

static constexpr size_t max_consecutive_reads = 20;
static constexpr size_t max_consecutive_writes = 20;

/// Manages the lifetime of a socket.
template <class Application>
class stream_manager : public socket_manager {
public:
  template <class... Ts>
  stream_manager(stream_socket handle, multiplexer* mpx, Ts&&... xs)
    : socket_manager(handle, mpx), application_(std::forward<Ts>(xs)...) {
    // nop
  }

  error init() {
    if (!nonblocking(handle(), true))
      return error(socket_operation_failed, "Could not set nonblocking");
    return application_.init(*this);
  }

  // -- socket_manager API -----------------------------------------------------

  bool handle_read_event() override {
    for (size_t i = 0; i < max_consecutive_reads; ++i) {
      auto data = recv_buffer_.data() + received_;
      auto size = recv_buffer_.size() - received_;
      auto read_res = read(handle<stream_socket>(), std::span(data, size));
      if (read_res > 0) {
        received_ += read_res;
        if (received_ == recv_buffer_.size()) {
          if (!application_.consume(*this, recv_buffer_)) {
            return false;
          }
        }
      } else if (read_res == 0) {
        return false;
      } else if (read_res < 0) {
        if (!last_socket_error_is_temporary()) {
          mpx()->handle_error(
            error(socket_operation_failed,
                  "[socket_manager.read()] " + last_socket_error_as_string()));
          return false;
        }
        return true;
      }
    }
    return true;
  }

  bool handle_write_event() override {
    // Handles writing data to the socket.
    // Returns `true` if everything has been written and `false` in case of an
    // error.
    auto write_some = [&]() {
      for (size_t i = 0; i < max_consecutive_writes; ++i) {
        auto num_bytes = send_buffer_.size() - written_;
        auto write_res = write(handle<stream_socket>(),
                               std::span(send_buffer_.data(), num_bytes));
        if (write_res > 0) {
          send_buffer_.erase(send_buffer_.begin(),
                             send_buffer_.begin() + write_res);
          if (send_buffer_.empty())
            break;
        } else {
          if (!last_socket_error_is_temporary())
            mpx()->handle_error(error(socket_operation_failed,
                                      "[socket_manager.write()] "
                                        + last_socket_error_as_string()));
          return false;
        }
      }
      return !send_buffer_.empty();
    };
    auto fetch_data = [&]() {
      for (size_t i = 0; i < 10 && application_.has_more_data(); ++i) {
        application_.produce(*this);
      }
    };
    fetch_data();
    return write_some() || application_.has_more_data();
  }

  bool handle_timeout(uint64_t) {
    mpx()->handle_error(error(
      runtime_error, "[stream_manager::handle_timeout()] not implemented!"));
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
