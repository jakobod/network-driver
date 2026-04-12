/**
 *  @author    Jakob Otto
 *  @file      layer.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "util/byte_buffer.hpp"

#include <chrono>

namespace net::detail {

/// @brief Interface for protocol layers in a network stack.
/// Defines the protocol contract between transport and application layers,
/// supporting bidirectional data flow with timeout management.
struct layer_base {
  /// @brief Virtual destructor for proper cleanup in derived classes.
  virtual ~layer_base() = default;

  // -- Upfacing interface (towards application) -------------------------------

  /// @brief Initializes the layer with configuration settings.
  /// @param The configuration object containing layer-specific settings.
  /// @return An error if initialization fails, success otherwise.
  virtual util::error init(const util::config&) = 0;

  /// @brief Checks if this layer has more data to send.
  /// @return True if there is pending data in the layer.
  virtual bool has_more_data() = 0;

  /// @brief Produces and enqueues more data for transmission.
  /// Called by transport when write buffer space is available.
  /// @return The result of the produce operation (ok, done, or error).
  virtual event_result produce() = 0;

  /// @brief Consumes received data for processing.
  /// Called by transport when data is available from the remote peer.
  /// @param bytes The received data to process.
  /// @return The result of the consume operation (ok, done, or error).
  virtual event_result consume(util::const_byte_span bytes) = 0;

  /// @brief Handles a timeout event.
  /// Called when a previously scheduled timeout expires.
  /// @param id The timeout identifier.
  /// @return The result of the timeout handling (ok, done, or error).
  virtual event_result handle_timeout(uint64_t id) = 0;

  // -- Downfacing interface (towards transport) -------------------------------

  /// @brief Configures the expected size of the next read operation.
  /// @param policy The receive policy with min/max size constraints.
  virtual void configure_next_read(receive_policy policy) = 0;

  /// @brief Returns a reference to the write buffer for enqueuing data.
  /// @return A reference to the output buffer.
  virtual util::byte_buffer& write_buffer() = 0;

  /// @brief Enqueues data for transmission via transport.
  /// @param bytes The data to send.
  virtual void enqueue(util::const_byte_span bytes) = 0;

  /// @brief Registers this layer for write readiness events.
  /// Notifies the multiplexer to monitor for write availability.
  virtual void register_writing() = 0;

  /// @brief Schedules a timeout after the specified duration.
  /// @param duration The timeout delay in milliseconds.
  /// @return A timeout identifier for cancellation or reference.
  virtual uint64_t set_timeout_in(std::chrono::milliseconds duration) = 0;

  /// @brief Schedules a timeout at the specified absolute time point.
  /// @param point The time point when the timeout should occur.
  /// @return A timeout identifier for cancellation or reference.
  virtual uint64_t set_timeout_at(std::chrono::system_clock::time_point point)
    = 0;
};

} // namespace net::detail
