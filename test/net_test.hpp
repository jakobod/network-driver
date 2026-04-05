/**
 *  @author    Jakob Otto
 *  @file      net_test.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "util/error_or.hpp"

#include <gtest/gtest.h>

#include <iostream>
#include <utility>

namespace net::test::detail {

/// @brief Helper to extract and validate error_or values in tests.
/// Unpacks an error_or<T> variant, asserting the value exists and fails
/// the test if an error occurred. Implicit conversion allows natural usage.
/// @tparam T The value type being unpacked.
template <class T>
struct unpacker {
  /// @brief Constructs unpacker from an error_or<T> variant.
  /// Fails the test if the variant contains an error, otherwise extracts value.
  /// @param result The error_or<T> to unpack.
  explicit unpacker(const util::error_or<T>& result) {
    if (auto err = util::get_error(result)) {
      ADD_FAILURE() << "Failed to unpack value: " << err;
    } else {
      value_ = util::get_value(result);
    }
  }

  /// @brief Move constructor form for rvalue error_or references.
  explicit unpacker(util::error_or<T>&& result) {
    if (auto err = util::get_error(result)) {
      ADD_FAILURE() << "Failed to unpack value: " << err;
    } else {
      value_ = std::move(util::get_value(result));
    }
  }

  /// @brief Implicit conversion to the value type.
  /// Allows the unpacker to be used directly as a T.
  /// @return The unpacked value.
  T unpack() && { return std::move(value_); }

private:
  T value_;
};

} // namespace net::test::detail

// -- Helper macros for tests --------------------------------------------------

/// @brief Streams a colored info message for test output.
#define MESSAGE() std::cerr << "\033[0;33m" << "[   info   ] " << "\033[m"

/// @brief Asserts an error_or result contains a value (not an error).
/// Expects the expression to not have an error; fails with message if it does.
/// @param expr The expression returning error_or<T> to check.
#define EXPECT_NO_ERROR(expr)                                                  \
  do {                                                                         \
    auto err = (expr);                                                         \
    EXPECT_FALSE(err.is_error())                                               \
      << #expr << " returned an error: " << err << std::endl;                  \
  } while (false)

/// @brief Asserts an error_or result contains a value; fails entire test if
/// error. Asserts (stronger than EXPECT) that the expression succeeds.
/// @param expr The expression returning error_or<T> to check.
#define ASSERT_NO_ERROR(expr)                                                  \
  do {                                                                         \
    auto err = (expr);                                                         \
    ASSERT_FALSE(err.is_error())                                               \
      << #expr << " returned an error: " << err << std::endl;                  \
  } while (false)

/// @brief Expects an error_or result contains an error (not a value).
/// Expects the expression to have an error; fails with message if it doesn't.
/// @param expr The expression returning error_or<T> to check.
#define EXPECT_ERROR(expr)                                                     \
  do {                                                                         \
    auto err = (expr);                                                         \
    EXPECT_TRUE(err.is_error())                                                \
      << #expr << " did NOT return an error: " << err << std::endl;            \
  } while (false)

/// @brief Asserts an error_or result contains an error; fails entire test if
/// value. Asserts (stronger than EXPECT) that the expression fails.
/// @param expr The expression returning error_or<T> to check.
#define ASSERT_ERROR(expr)                                                     \
  do {                                                                         \
    auto err = (expr);                                                         \
    ASSERT_TRUE(err.is_error())                                                \
      << #expr << " did NOT return an error: " << err << std::endl;            \
  } while (false)

/// @brief Unpacks an error_or variable, extracting the value and failing if
/// error. Usage: `auto value = UNPACK_VARIABLE(error_or_result);` The unpacker
/// validates the result and extracts the value implicitly.
#define UNPACK_VARIABLE(var) net::test::detail::unpacker{var}.unpack()

/// @brief Executes an expression and unpacks its error_or result.
/// Convenience macro combining expression execution with unpacking.
/// Usage: `auto value = UNPACK_EXPRESSION(some_function());`
#define UNPACK_EXPRESSION(expr) net::test::detail::unpacker{expr}.unpack()
