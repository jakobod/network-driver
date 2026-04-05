/**
 *  @author    Jakob Otto
 *  @file      exception.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "util/error.hpp"
#include "util/error_code.hpp"
#include "util/format.hpp"

#include <exception>
#include <string>

namespace util {

/// @brief Exception class for library errors with structured error codes.
/// Combines an error_code with a detailed error message in a standard C++
/// exception type. Useful when exceptions are preferred over error_or<T>
/// return types for error handling.
class exception : public std::exception {
  friend class error;

public:
  /// @brief Constructs an exception with an error code and message.
  /// @param code The error code categorizing the error.
  /// @param reason The human-readable error reason/message.
  exception(error_code code, std::string reason) noexcept
    : code_{code}, reason_{std::move(reason)} {
    // nop
  }

  /// @brief Constructs an exception with just an error code.
  /// The reason is initialized to an empty string.
  /// @param code The error code.
  explicit exception(error_code code) noexcept : code_{code} {
    // nop
  }

  /// @brief Constructs an exception with just an error code.
  /// The reason is initialized to an empty string.
  /// @param code The error code.
  explicit exception(const error& err) noexcept
    : code_{err.code_}, reason_{err.msg_} {
    // nop
  }

  explicit exception(error&& err) noexcept
    : code_{std::move(err.code_)}, reason_{std::move(err.msg_)} {
    // nop
  }

  /// @brief Default destructor.
  virtual ~exception() = default;

  /// @brief Returns the error code associated with this exception.
  /// @return The error_code enum value.
  error_code code() const noexcept { return code_; }

  /// @brief Returns the human-readable error reason/message.
  /// @return A const reference to the reason string.
  std::string_view reason() const noexcept { return reason_; }

  /// @brief Returns a C-string describing the exception.
  /// Combines the error code and reason into a single message.
  /// @return A C-string with the complete error description.
  const char* what() const noexcept override { return reason_.c_str(); }

private:
  error_code code_{error_code::runtime_error};
  std::string reason_;
};

} // namespace util
