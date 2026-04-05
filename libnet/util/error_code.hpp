/**
 *  @author    Jakob Otto
 *  @file      error_code.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <cstdint>
#include <string_view>

namespace util {

/// @brief Enumeration of error codes for categorizing runtime errors.
/// Used with the error class to provide structured error information
/// across the library, enabling programmatic error handling.
enum class error_code : std::uint8_t {
  /// No error occurred (success condition).
  no_error = 0,
  /// Generic runtime error without specific categorization.
  runtime_error,
  /// A socket operation failed (send, recv, bind, listen, etc.).
  socket_operation_failed,
  /// Invalid argument provided to a function.
  invalid_argument,
  /// Parsing failed (e.g., CLI parser, configuration parser).
  parser_error,
  /// OpenSSL/TLS operation failed.
  openssl_error,
};

/// @brief Converts an error code to its string representation.
/// @param err The error code to stringify.
/// @return A human-readable string name for the error code.
constexpr std::string_view to_string(error_code err) noexcept {
  switch (err) {
    case error_code::no_error:
      return "no_error";
    case error_code::runtime_error:
      return "runtime_error";
    case error_code::socket_operation_failed:
      return "socket_operation_failed";
    case error_code::invalid_argument:
      return "invalid_argument";
    case error_code::parser_error:
      return "parser_error";
    case error_code::openssl_error:
      return "openssl_error";
    default:
      return "???";
  }
}

} // namespace util
