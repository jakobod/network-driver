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

#include "util/byte_buffer.hpp"
#include "util/byte_span.hpp"
#include "util/config.hpp"
#include "util/error.hpp"
#include "util/logger.hpp"

namespace net {

/// @brief Base class for layered transport protocols.
/// Provides a generic transport layer abstraction with buffer management
/// for both reading and writing operations. Derived classes implement
/// specific protocol handling (TCP, UDP, etc.) and can stack multiple
/// protocol layers on top of this base.
class transport : public detail::event_handler {
public:
  /// @brief Constructs a transport with a socket and multiplexer.
  /// @param handle The socket managed by this transport.
  /// @param mpx The multiplexer managing I/O events for this socket.
  transport(socket handle, detail::multiplexer_base* mpx)
    : detail::event_handler{handle, mpx} {
    // nop
  }

  /// @brief Initializes the transport layer with configuration.
  /// Sets up tuneable parameters from the configuration object.
  /// @param cfg The configuration object with transport settings.
  /// @return An error if initialization fails, success otherwise.
  util::error init(const util::config& cfg) override {
    LOG_TRACE();
    max_consecutive_fetches_ = cfg.get_or("transport.max-consecutive-fetches",
                                          std::int64_t{10});
    max_consecutive_reads_ = cfg.get_or("transport.max-consecutive-reads",
                                        std::int64_t{20});
    max_consecutive_writes_ = cfg.get_or("transport.max-consecutive-writes",
                                         std::int64_t{20});
    LOG_DEBUG(NET_ARG(max_consecutive_fetches_),
              NET_ARG(max_consecutive_reads_),
              NET_ARG(max_consecutive_writes_));
    return util::none;
  }

  // -- public API -------------------------------------------------------------

  /// @brief Configures the read buffer for the next read operation.
  /// Subclasses must implement this to handle protocol-specific read setup.
  /// @param policy The receive policy specifying min and max read sizes.
  virtual void configure_next_read(receive_policy policy) = 0;

  /// @brief Returns a reference to the write buffer.
  /// Data enqueued to this buffer will be sent over the socket.
  /// @return A reference to the write buffer.
  util::byte_buffer& write_buffer() { return write_buffer_; }

  /// @brief Enqueues data to be written.
  /// Appends bytes to the write buffer for transmission.
  /// @param bytes The data to enqueue for sending.
  void enqueue(util::const_byte_span bytes) {
    LOG_TRACE();
    // TODO: copying the data is not very performant. Think of another solution
    write_buffer_.insert(write_buffer_.end(), bytes.begin(), bytes.end());
  }

protected:
  size_t max_consecutive_fetches_ = 10;
  size_t max_consecutive_reads_ = 20;
  size_t max_consecutive_writes_ = 20;

  size_t received_ = 0;
  size_t written_ = 0;
  size_t min_read_size_ = 0;

  util::byte_buffer read_buffer_;
  util::byte_buffer write_buffer_;
};

} // namespace net
