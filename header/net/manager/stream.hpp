/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/socket_manager.hpp"

#include <utility>

#include "fwd.hpp"
#include "net/error.hpp"
#include "net/multiplexer.hpp"
#include "net/receive_policy.hpp"
#include "net/stream_socket.hpp"

namespace net::manager {

/// Manages a stream oriented transport socket.
template <class Application>
class stream : public socket_manager {
  static constexpr size_t max_consecutive_fetches = 20;
  static constexpr size_t max_consecutive_reads = 20;
  static constexpr size_t max_consecutive_writes = 20;

public:
  template <class... Ts>
  stream(stream_socket handle, multiplexer* mpx, Ts&&... xs)
    : socket_manager(handle, mpx), application_(std::forward<Ts>(xs)...) {
    // nop
  }

  error init() override {
    return application_.init(*this);
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
          if (application_.consume(*this,
                                   std::span(read_buffer_.data(), received_))
              == event_result::error)
            return event_result::error;
        }
      } else if (read_res == 0) {
        return event_result::error;
      } else if (read_res < 0) {
        if (!last_socket_error_is_temporary()) {
          handle_error(
            error(socket_operation_failed,
                  "[socket_manager.read()] " + last_socket_error_as_string()));
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
          handle_error(
            error(socket_operation_failed,
                  "[stream::write()] " + last_socket_error_as_string()));

          return event_result::error;
        }
      }
    }
    return done_writing() ? event_result::done : event_result::ok;
  }

  event_result handle_data(util::const_byte_span data) override {
    // TODO this is an unnessecary copy.
    read_buffer_.insert(read_buffer_.end(), data.begin(), data.end());
    if (read_buffer_.size() >= min_read_size_) {
      if (application_.consume(*this, read_buffer_) == event_result::error)
        return event_result::error;
      read_buffer_.clear();
    }
    return event_result::ok;
  }

  event_result write_data(util::const_byte_span data) override {
    auto done_writing = [&]() { return !application_.has_more_data(); };
    auto fetch = [&]() {
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
          handle_error(
            error(socket_operation_failed,
                  "[stream::write()] " + last_socket_error_as_string()));

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
  void configure_next_read(receive_policy policy) {
    received_ = 0;
    min_read_size_ = policy.min_size;
    if (read_buffer_.size() != policy.max_size)
      read_buffer_.resize(policy.max_size);
  }

  /// Returns a reference to the send_buffer
  util::byte_buffer& write_buffer() {
    return write_buffer_;
  }

private:
  Application application_;

  size_t received_ = 0;
  size_t written_ = 0;
  size_t min_read_size_ = 0;

  util::byte_buffer read_buffer_;
  util::byte_buffer write_buffer_;
};

} // namespace net::manager
