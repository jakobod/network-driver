/**
 *  @author    Jakob Otto
 *  @file      error_or.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "util/error.hpp"

#include <variant>

namespace util {

/// @brief Template alias for a value-or-error return type.
/// error_or<T> is a variant that holds either a value of type T
/// or an error object. This allows propagating success and failure
/// information without exceptions.
/// @tparam T The type of the success value.
template <class T>
using error_or = std::variant<T, error>;

/// @brief Extracts error pointer from mutable error_or variant.
/// Returns nullptr if the variant holds a value, or a pointer to the
/// error object if it holds an error.
/// @tparam T The value type in the error_or variant.
/// @param maybe_error The variant to check.
/// @return Pointer to the error if present, nullptr otherwise.
template <class T>
error* get_error(error_or<T>& maybe_error) {
  return std::get_if<error>(&maybe_error);
}

/// @brief Extracts error pointer from const error_or variant.
/// Returns nullptr if the variant holds a value, or a pointer to the
/// error object if it holds an error (const version).
/// @tparam T The value type in the error_or variant.
/// @param maybe_error The variant to check.
/// @return Const pointer to the error if present, nullptr otherwise.
template <class T>
const error* get_error(const error_or<T>& maybe_error) {
  return std::get_if<error>(&maybe_error);
}

/// @brief Extracts the value from an error_or variant.
/// Asserts that the variant holds a value, not an error (in debug builds).
/// @tparam T The value type in the error_or variant.
/// @param maybe_error The variant to extract from.
/// @return Const reference to the value.
template <class T>
const T& get_value(const error_or<T>& maybe_error) {
  return std::get<T>(maybe_error);
}

/// @brief Extracts the value from an error_or variant (mutable version).
/// Asserts that the variant holds a value, not an error (in debug builds).
/// @tparam T The value type in the error_or variant.
/// @param maybe_error The variant to extract from.
/// @return Reference to the value.
template <class T>
T& get_value(error_or<T>& maybe_error) {
  return std::get<T>(maybe_error);
}

} // namespace util
