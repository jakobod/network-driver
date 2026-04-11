/**
 *  @author    Jakob Otto
 *  @file      manager_result.hpp
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
enum class manager_result : std::uint8_t {
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
/// @param op The manager_result to stringify.
/// @return A string describing the result ("ok", "done", or "error").
constexpr std::string_view to_string(manager_result res) noexcept {
  switch (res) {
    case manager_result::ok:
      return "manager_result::ok";
    case manager_result::done:
      return "manager_result::done";
    case manager_result::error:
      return "manager_result::error";
    case manager_result::temporary_error:
      return "manager_result::temporary_error";
    default:
      return "manager_result::unknown";
  }
}

} // namespace net
