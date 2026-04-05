/**
 *  @author    Jakob Otto
 *  @file      event_result.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <cstdint>
#include <string>

namespace net {

/// @brief Result type for I/O event handling operations.
/// Indicates the outcome of handling a socket event (read/write).
enum class event_result : std::uint8_t {
  /// Operation succeeded and can continue.
  ok = 0,
  /// Operation completed successfully and handler is done (no more work).
  done,
  /// Operation failed with an error condition.
  error,
};

/// @brief Converts an event result to its string representation.
/// @param op The event_result to stringify.
/// @return A string describing the result ("ok", "done", or "error").
std::string to_string(event_result op);

} // namespace net
