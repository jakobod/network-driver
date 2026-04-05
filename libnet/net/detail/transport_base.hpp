/**
 *  @author    Jakob Otto
 *  @file      transport.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "net/detail/event_handler.hpp"

#include "net/ip/v4_endpoint.hpp"
#include "net/receive_policy.hpp"

#include "util/assert.hpp"
#include "util/byte_buffer.hpp"
#include "util/byte_span.hpp"
#include "util/config.hpp"
#include "util/error.hpp"

namespace net::detail {

/// @brief Base class for layered transport protocols.
/// Provides a generic transport layer abstraction with buffer management
/// for both reading and writing operations. Derived classes implement
/// specific protocol handling (TCP, UDP, etc.) and can stack multiple
/// protocol layers on top of this base.
class transport_base {
public:
  /// @brief Constructs a transport_base with a socket and multiplexer.
  /// @param handle The socket managed by this transport_base.
  /// @param mpx The multiplexer managing I/O events for this socket.
  transport_base() noexcept = default;

  /// @brief Initializes the transport_base layer with configuration.
  /// Sets up tuneable parameters from the configuration object.
  /// @param cfg The configuration object with transport_base settings.
  /// @return An error if initialization fails, success otherwise.
  util::error init(const util::config& cfg) {
    max_consecutive_fetches_ = cfg.get_or("transport.max-consecutive-fetches",
                                          std::int64_t{10});
    max_consecutive_reads_ = cfg.get_or("transport.max-consecutive-reads",
                                        std::int64_t{20});
    max_consecutive_writes_ = cfg.get_or("transport.max-consecutive-writes",
                                         std::int64_t{20});
    return util::none;
  }

  /// @brief Configures the read buffer for the next read operation.
  /// @param policy The receive policy specifying min and max read sizes.
  virtual void configure_next_read(receive_policy policy) noexcept = 0;

  virtual void enqueue(util::const_byte_span) {
    ASSERT(false, "This is a default implementation");
  }

  virtual void enqueue(util::const_byte_span, net::ip::v4_endpoint) {
    ASSERT(false, "This is a default implementation");
  }

protected:
  size_t max_consecutive_fetches_ = 10;
  size_t max_consecutive_reads_ = 20;
  size_t max_consecutive_writes_ = 20;
};

} // namespace net::detail
