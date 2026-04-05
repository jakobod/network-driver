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
  /// Operation failed with a temporary error condition, i.e., EAGAIN,
  /// EWOULDBLOCK, etc.
  temporary_error,
};

/// @brief Converts an event result to its string representation.
/// @param op The event_result to stringify.
/// @return A string describing the result ("ok", "done", or "error").
constexpr std::string_view to_string(event_result res) noexcept {
  switch (res) {
    case event_result::ok:
      return "event_result::ok";
    case event_result::done:
      return "event_result::done";
    case event_result::error:
      return "event_result::error";
    case event_result::temporary_error:
      return "event_result::temporary_error";
    default:
      return "event_result::unknown";
  }
}

} // namespace net
