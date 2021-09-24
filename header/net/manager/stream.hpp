/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/socket_manager.hpp"

#include <memory>
#include <utility>

#include "fwd.hpp"
#include "net/error.hpp"
#include "net/multiplexer.hpp"
#include "net/receive_policy.hpp"
#include "net/socket.hpp"
#include "net/socket_manager_factory.hpp"
#include "net/stream_socket.hpp"
#include "util/scope_guard.hpp"

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
    // TODO: This is for debugging multithreaded runs.
    if (!read_lock_.try_lock()) {
      std::cerr << "MULTIPLE READING" << std::endl;
      return event_result::done;
    }
    util::scope_guard([&]() { read_lock_.unlock(); });
    for (size_t i = 0; i < max_consecutive_reads; ++i) {
      auto data = read_buffer_.data() + received_;
      auto size = read_buffer_.size() - received_;
      auto read_res = read(handle<stream_socket>(), std::span(data, size));
      if (read_res > 0) {
        received_ += read_res;
        if (received_ >= min_read_size_) {
          if (!application_.consume(*this,
                                    std::span(read_buffer_.data(), received_)))
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
    // TODO: This is for debugging multithreaded runs.
    if (!write_lock_.try_lock()) {
      std::cerr << "MULTIPLE WRITING" << std::endl;
      return event_result::done;
    }
    util::scope_guard([&]() { write_lock_.unlock(); });
    auto done_writing = [&]() {
      return (write_buffer_.empty() && !application_.produce(*this));
    };
    if (done_writing())
      return event_result::done;
    for (size_t i = 0; i < max_consecutive_writes; ++i) {
      auto write_res = write(handle<stream_socket>(), write_buffer_);
      if (write_res > 0) {
        write_buffer_.erase(write_buffer_.begin(),
                            write_buffer_.begin() + write_res);
        if (done_writing())
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
  std::mutex buffer_lock_;
  Application application_;

  size_t received_ = 0;
  size_t min_read_size_ = 0;

  util::byte_buffer read_buffer_;
  util::byte_buffer write_buffer_;
  util::byte_buffer data_buffer_;

  std::mutex read_lock_;
  std::mutex write_lock_;
};

template <class Application>
class stream_factory : public socket_manager_factory {
public:
  socket_manager_ptr make(socket handle, multiplexer* mpx) override {
    using manager_type = stream<Application>;
    return std::make_shared<manager_type>(socket_cast<stream_socket>(handle),
                                          mpx);
  };
};

} // namespace net::manager
