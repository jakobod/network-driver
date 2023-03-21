/**
 *  @author    Jakob Otto
 *  @file      stream_transport.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "net/event_result.hpp"
#include "net/receive_policy.hpp"
#include "net/stream_socket.hpp"
#include "net/transport.hpp"

#include "util/error.hpp"
#include "util/format.hpp"
#include "util/logger.hpp"

#include <utility>

namespace net {

/// Implements a stream oriented transport.
template <class NextLayer>
class stream_transport : public transport {
public:
  template <class... Ts>
  stream_transport(stream_socket handle, multiplexer* mpx, Ts&&... xs)
    : transport(handle, mpx), next_layer_(*this, std::forward<Ts>(xs)...) {
    LOG_DEBUG("Creating stream_transport with ", NET_ARG2("id", handle.id));
  }

  util::error init() override {
    LOG_DEBUG("init stream_transport with ", NET_ARG2("socket", handle().id));
    if (!nonblocking(handle(), true))
      return {util::error_code::runtime_error,
              util::format("Failed to set nonblocking on sock={0}",
                           handle().id)};
    return next_layer_.init();
  }

  // -- socket_manager API -----------------------------------------------------

  event_result handle_read_event() override {
    LOG_TRACE();
    LOG_DEBUG("handle read_event on ", NET_ARG2("socket", handle().id));
    for (size_t i = 0; i < max_consecutive_reads; ++i) {
      auto data = read_buffer_.data() + received_;
      auto size = read_buffer_.size() - received_;
      auto read_res = read(handle<stream_socket>(), std::span(data, size));
      if (read_res > 0) {
        LOG_DEBUG("Read ", read_res, " bytes from ",
                  NET_ARG2("socket", handle().id));
        received_ += read_res;
        if (received_ >= min_read_size_) {
          if (next_layer_.consume(
                util::const_byte_span{read_buffer_.data(), received_})
              == event_result::error)
            return event_result::error;
          else
            received_ = 0; // Data should be consumed completely
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
    LOG_TRACE();
    LOG_DEBUG("handle write_event on ", NET_ARG2("socket", handle().id));
    auto done_writing = [&]() {
      return (write_buffer_.empty() && !next_layer_.has_more_data());
    };
    auto fetch = [&]() {
      for (size_t i = 0;
           next_layer_.has_more_data() && (i < max_consecutive_fetches); ++i)
        next_layer_.produce();
      return !write_buffer_.empty();
    };
    if (write_buffer_.empty())
      if (!fetch())
        return event_result::done;
    for (size_t i = 0; i < max_consecutive_writes; ++i) {
      auto write_res = write(handle<stream_socket>(), write_buffer_);
      if (write_res > 0) {
        LOG_DEBUG("Wrote ", write_res, " bytes to ",
                  NET_ARG2("socket", handle().id));
        write_buffer_.erase(write_buffer_.begin(),
                            write_buffer_.begin() + write_res);
        if (write_buffer_.empty())
          if (!fetch())
            return event_result::done;
      } else {
        if (last_socket_error_is_temporary()) {
          return event_result::ok;
        } else {
          handle_error({util::error_code::socket_operation_failed,
                        util::format("[stream::write()] errno = {0}: {1}",
                                     errno, last_socket_error_as_string())});
          return event_result::error;
        }
      }
    }
    return done_writing() ? event_result::done : event_result::ok;
  }

  event_result handle_timeout(uint64_t id) override {
    return next_layer_.handle_timeout(id);
  }

  // -- public API -------------------------------------------------------------

  /// Configures the amount to be read next
  void configure_next_read(receive_policy policy) override {
    received_ = 0;
    min_read_size_ = policy.min_size;
    if (read_buffer_.size() != policy.max_size)
      read_buffer_.resize(policy.max_size);
    LOG_DEBUG("Configuring next read on ", NET_ARG2("socket", handle().id),
              ": ", NET_ARG(min_read_size_), ", ",
              NET_ARG2("max_read_size_", policy.max_size));
  }

private:
  NextLayer next_layer_;
};

} // namespace net
