/**
 *  @author    Jakob Otto
 *  @file      timeout_entry.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/socket/socket_id.hpp"

#include <chrono>
#include <cstdint>

namespace net {

/// @brief Represents a scheduled timeout event.
/// Stores the socket, expiration time, and identifier for a timeout
/// that can be tracked and triggered by the multiplexer.
struct timeout_entry {
  /// @brief Type alias for timeout identifiers.
  using timeout_id = uint64_t;

  /// @brief Constructs a timeout entry.
  /// @param handle The socket associated with this timeout.
  /// @param when The time point when the timeout expires.
  /// @param id The unique identifier for this timeout.
  timeout_entry(socket_id handle, std::chrono::steady_clock::time_point when,
                timeout_id id)
    : handle_{handle}, when_{when}, id_{id} {
    // nop
  }

  // -- properties -------------------------------------------------------------

  /// @brief Returns the socket handle for this timeout.
  /// @return The socket ID associated with this timeout.
  socket_id handle() const noexcept { return handle_; }

  /// @brief Returns the expiration time of this timeout.
  /// @return The steady_clock time point when timeout expires.
  std::chrono::steady_clock::time_point when() const noexcept { return when_; }

  /// @brief Returns the timeout identifier.
  /// @return The unique ID for this timeout.
  timeout_id id() const noexcept { return id_; }

  /// @brief Checks if this timeout has expired.
  /// Compares the expiration time against the current system time.
  /// @return True if the current time is past the expiration time.
  bool has_expired() const noexcept {
    return when_ <= std::chrono::steady_clock::now();
  }

  // -- Comparison operations --------------------------------------------------

  /// @brief Less-than comparison for timeline ordering.
  /// @param other The timeout to compare with.
  /// @return True if this expires before the other timeout.
  bool operator<(const timeout_entry& other) const noexcept {
    return when_ < other.when_;
  }

  /// @brief Greater-than comparison for timeline ordering.
  /// @param other The timeout to compare with.
  /// @return True if this expires after the other timeout.
  bool operator>(const timeout_entry& other) const noexcept {
    return when_ > other.when_;
  }

  /// @brief Equality comparison.
  /// @param other The timeout to compare with.
  /// @return True if all fields are identical.
  bool operator==(const timeout_entry& other) const noexcept = default;

private:
  // -- members ----------------------------------------------------------------

  socket_id handle_; ///< The socket handle for this timeout.
  // TODO: Add a pointer to the manager that should be triggered
  std::chrono::steady_clock::time_point when_; ///< The expiration time point.
  timeout_id id_; ///< The unique timeout identifier.
};

} // namespace net
