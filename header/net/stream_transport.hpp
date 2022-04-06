/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include "net/receive_policy.hpp"
#include "net/stream_socket.hpp"
#include "net/transport.hpp"

#include "util/error.hpp"

#include <utility>

namespace net {

/// Implements a stream oriented transport.
template <class Application>
class stream_transport : public transport {
public:
  template <class... Ts>
  stream_transport(stream_socket handle, multiplexer* mpx, Ts&&... xs)
    : transport(handle, mpx), application_(*this, std::forward<Ts>(xs)...) {
    // nop
  }

  util::error init() override {
    return application_.init();
  }

  // -- socket_manager API -----------------------------------------------------

  event_result handle_read_event() override {
    for (size_t i = 0; i < max_consecutive_reads; ++i) {
      auto data = read_buffer_.data() + received_;
      auto size = read_buffer_.size() - received_;
      auto read_res = read(handle<stream_socket>(), std::span(data, size));
      if (read_res > 0) {
        received_ += read_res;
        if (received_ >= min_read_size_) {
          if (application_.consume(
                util::const_byte_span{read_buffer_.data(), received_})
              == event_result::error)
            return event_result::error;
        }
      } else if (read_res == 0) {
        return event_result::error;
      } else if (read_res < 0) {
        if (!last_socket_error_is_temporary()) {
          handle_error(util::error(util::error_code::socket_operation_failed,
                                   "[socket_manager.read()] "
                                     + last_socket_error_as_string()));
          return event_result::error;
        }
        return event_result::ok;
      }
    }
    return event_result::ok;
  }

  event_result handle_write_event() override {
    auto done_writing = [&]() {
      return (write_buffer_.empty() && !application_.has_more_data());
    };
    auto fetch = [&]() {
      auto fetched = application_.has_more_data();
      for (size_t i = 0;
           application_.has_more_data() && (i < max_consecutive_fetches); ++i)
        application_.produce();
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
          handle_error(
            util::error(util::error_code::socket_operation_failed,
                        "[stream::write()] " + last_socket_error_as_string()));

          return event_result::error;
        }
      }
    }
    return done_writing() ? event_result::done : event_result::ok;
  }

  event_result handle_timeout(uint64_t id) override {
    return application_.handle_timeout(id);
  }

  // -- public API -------------------------------------------------------------

  /// Configures the amount to be read next
  void configure_next_read(receive_policy policy) override {
    received_ = 0;
    min_read_size_ = policy.min_size;
    if (read_buffer_.size() != policy.max_size)
      read_buffer_.resize(policy.max_size);
  }

private:
  Application application_;
};

} // namespace net
