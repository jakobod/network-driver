/**
 *  @author    Jakob Otto
 *  @file      error.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "util/error_code.hpp"
#include "util/format.hpp"

#include <ostream>
#include <string>

namespace util {

/// @brief Error object encapsulating an error code and optional message.
/// This class represents an error condition with both a structured error code
/// and a human-readable message. It can be used in return types via error_or
/// to represent failure conditions.
class [[nodiscard]] error {
  friend class exception;

public:
  /// @brief Constructs an error with a code and message.
  /// @param code The error code categorizing the error.
  /// @param msg The human-readable error message.
  error(error_code code, std::string msg);

  /// @brief Constructs an error with a code and formatted message.
  /// The message is formatted using the provided format string and arguments,
  /// similar to printf-style formatting.
  /// @tparam Arguments The types of format arguments.
  /// @param code The error code categorizing the error.
  /// @param form The format string for the message.
  /// @param args Format arguments to substitute into the format string.
  template <class... Arguments>
  error(error_code code, std::string form, Arguments... args)
    : error{code, format(std::move(form), std::move(args)...)} {
    // nop
  }

  /// @brief Constructs an error with just a code.
  /// The message is empty initially but can be set elsewhere.
  /// @param code The error code.
  error(error_code code);

  /// @brief Default constructor creating a no-error state.
  error() = default;

  /// @brief Destructor.
  ~error() = default;

  /// @brief Defaulted copy construction
  error(const error&) = default;
  /// @brief Defaulted copy assignment
  error& operator=(const error&) = default;
  /// @brief Defaulted copy construction
  error(error&&) noexcept = default;
  /// @brief Defaulted move assignment
  error& operator=(error&&) noexcept = default;

  // -- accessors --------------------------------------------------------------

  /// @brief Retrieves the error code.
  /// @return The error_code enum value.
  error_code code() const noexcept { return code_; }

  /// @brief Retrieves the error message.
  /// @return A const reference to the message string.
  const std::string& msg() const { return msg_; }

  // -- boolean operators ------------------------------------------------------

  /// @brief Checks if this represents an error condition.
  /// @return True if code() != error_code::no_error, false otherwise.
  bool is_error() const noexcept;

  /// @brief Boolean conversion operator.
  /// Allows using error directly in boolean contexts (true if error occurred).
  /// @return True if this is an error, false otherwise.
  operator bool() const noexcept;

  /// @brief Equality comparison.
  /// @param other The error to compare with.
  /// @return True if both have the same code and message.
  bool operator==(const error& other) const noexcept;

  /// @brief Inequality comparison.
  /// @param other The error to compare with.
  /// @return True if they differ in code or message.
  bool operator!=(const error& other) const noexcept;

  /// @brief Stream output operator.
  /// @param os The output stream.
  /// @param err The error to output.
  /// @return The stream for chaining.
  friend std::ostream& operator<<(std::ostream& os, const error& err) {
    return os << to_string(err.code()) << std::string(": ") << err.msg();
  }

private:
  error_code code_{error_code::no_error};
  std::string msg_;
};

/// @brief Predefined constant representing no error condition.
static const error none{};

/// @brief Converts an error object to its string representation.
/// @param err The error to stringify.
/// @return A formatted string containing the error code name and message.
std::string to_string(const error& err);

/// @brief Converts the system errno to a human-readable error string.
/// Retrieves the current value of errno and converts it to a descriptive
/// string.
/// @return The error string for the current errno value.
std::string last_error_as_string();

} // namespace util
