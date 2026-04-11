/**
 *  @author    Jakob Otto
 *  @file      application.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

namespace net {

/// @brief Interface for application-level protocol implementations.
/// Defines the contract for custom protocol handlers that operate above
/// the transport layer, handling data production and consumption.
struct application {
  /// @brief Virtual destructor for proper cleanup in derived classes.
  virtual ~application() = default;

  /// @brief Initializes the application layer with configuration.
  /// @param cfg The configuration object containing application settings.
  /// @return An error if initialization fails, success otherwise.
  virtual util::error init(const util::config& cfg) = 0;

  /// @brief Checks if the application has more data to send.
  /// @return True if there is pending data waiting to be transmitted.
  virtual bool has_more_data() = 0;

  /// @brief Produces and sends more application data.
  /// Called when the transport is ready to accept more bytes.
  /// @return The result of the produce operation (ok, done, or error).
  virtual manager_result produce() = 0;

  /// @brief Consumes and processes received data.
  /// Called when data arrives from the remote peer via the transport.
  /// @param bytes The received data to process.
  /// @return The result of consumption (ok, done, or error).
  virtual manager_result consume(util::const_byte_span bytes) = 0;

  /// @brief Handles an application-level timeout.
  /// Called when a timer previously registered by the application expires.
  /// @param id The timeout identifier that triggered.
  /// @return The result of timeout handling (ok, done, or error).
  virtual manager_result handle_timeout(uint64_t id) = 0;
};

} // namespace net
