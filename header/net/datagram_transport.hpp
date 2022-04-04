/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/socket_manager.hpp"

#include <utility>

#include "fwd.hpp"
#include "net/error.hpp"
#include "net/receive_policy.hpp"
#include "net/udp_datagram_socket.hpp"

namespace net {

/// Implements a datagram oriented transport.
template <class Application>
class datagram_transport : public socket_manager {
  static constexpr const std::size_t max_datagram_size = 576;

  static constexpr const size_t max_consecutive_fetches = 20;
  static constexpr const size_t max_consecutive_reads = 20;
  static constexpr const size_t max_consecutive_writes = 20;

  using packet_buffer = util::byte_array<max_datagram_size>;

public:
  template <class... Ts>
  datagram_transport(udp_datagram_socket handle, multiplexer* mpx, Ts&&... xs)
    : socket_manager(handle, mpx), application_(std::forward<Ts>(xs)...) {
    // nop
  }

  error init() override {
    return application_.init(*this);
  }

  // -- socket_manager API -----------------------------------------------------

  event_result handle_read_event() override {
    for (size_t i = 0; i < max_consecutive_reads; ++i) {
      auto [source_ep, read_res] = read(handle<udp_datagram_socket>(),
                                        read_buffer_);
      if (read_res > 0) {
        received_ += read_res;
        if (received_ >= min_read_size_) {
          if (application_.consume(*this, source_ep,
                                   std::span(read_buffer_.data(), read_res))
              == event_result::error)
            return event_result::error;
        }
      } else if (read_res < 0) {
        if (!last_socket_error_is_temporary()) {
          handle_error(
            error(socket_operation_failed,
                  "[socket_manager.read()] " + last_socket_error_as_string()));
          return event_result::error;
        }
        return event_result::ok;
      } else {
        // Empty packets are valid, but can be ignored
      }
    }
    return event_result::ok;
  }

  event_result handle_write_event() override {
    // Checks if this transport is done writing
    auto done_writing = [&write_buffer_, &application_]() -> bool {
      return (write_buffer_.empty() && !application_.has_more_data());
    };
    auto fetch = [&application_, &max_consecutive_fetches]() -> bool {
      auto fetched = application_.has_more_data();
      for (size_t i = 0;
           application_.has_more_data() && (i < max_consecutive_fetches); ++i)
        application_.produce(*this);
      return fetched;
    };

    if (write_buffer_.empty())
      if (!fetch())
        return event_result::done;
    for (size_t i = 0; i < max_consecutive_writes; ++i) {
      auto num_bytes = write_buffer_.size() - written_;
      auto write_res = write(handle<stream_socket>(),
                             std::span(write_buffer_.data(), num_bytes));
      if (write_res > 0) {
        write_buffer_.erase(write_buffer_.begin(),
                            write_buffer_.begin() + write_res);
        if (write_buffer_.empty())
          if (!fetch())
            return event_result::done;
      } else {
        if (last_socket_error_is_temporary()) {
          return event_result::ok;
        } else {
          handle_error(error(socket_operation_failed,
                             util::format("[datagram_transport::write()] {0}",
                                          last_socket_error_as_string())));
          return event_result::error;
        }
      }
    }
    return done_writing() ? event_result::done : event_result::ok;
  }

  event_result handle_timeout(uint64_t id) {
    return application_.handle_timeout(*this, id);
  }

  // -- public API -------------------------------------------------------------

  /// Configures the amount to be read next
  void configure_next_read(receive_policy) {
    // nop
  }

  /// Returns a reference to the send_buffer
  util::byte_buffer& write_buffer() {
    return write_buffer_;
  }

private:
  std::unordered_map<ip::v4_endpoint, Application> applications_;

  packet_buffer read_buffer_;
  util::byte_buffer write_buffer_;
};

} // namespace net
