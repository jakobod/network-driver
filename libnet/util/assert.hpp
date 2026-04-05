/**
 *  @author    Jakob Otto
 *  @file      assert.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#if defined(LIB_NET_ENABLE_ASSERTIONS)

#  include <cstdlib>
#  include <iostream>
#  include <sstream>

// -- Assertion implementation -------------------------------------------------

namespace util::detail {

/// @brief Assertion failure handler function.
/// Reports assertion failure with file, line, and condition information.
/// @param condition The stringified condition that failed
/// @param file The source file where assertion occurred
/// @param line The line number where assertion occurred
inline void assert_fail(const char* condition, const char* file, int line) {
  std::cerr << "\n=== ASSERTION FAILED ===\n"
            << "File: " << file << ":" << line << "\n"
            << "Condition: " << condition << "\n"
            << "======================\n"
            << std::endl;
  std::abort();
}

/// @brief Assertion failure handler function with custom message.
/// Reports assertion failure with file, line, condition, and debug message.
/// @param condition The stringified condition that failed
/// @param file The source file where assertion occurred
/// @param line The line number where assertion occurred
/// @param message Debug information message to display
inline void assert_fail(const char* condition, const char* file, int line,
                        const char* message) {
  std::cerr << "\n=== ASSERTION FAILED ===\n"
            << "File: " << file << ":" << line << "\n"
            << "Condition: " << condition << "\n"
            << "Message: " << message << "\n"
            << "======================\n"
            << std::endl;
  std::abort();
}

} // namespace util::detail

// -- Release assertions (always enabled) --------------------------------------

/// @brief Runtime assertion that aborts the program if condition is false.
/// Supports two forms:
///   ASSERT(cond) - basic assertion with no message
///   ASSERT(cond, "message") - assertion with debug message
/// @param cond The condition to check
#  define ASSERT_IMPL(cond)                                                    \
    do {                                                                       \
      if (!(cond)) {                                                           \
        ::util::detail::assert_fail(#cond, __FILE__, __LINE__);                \
      }                                                                        \
    } while (false)

#  define ASSERT_IMPL_MSG(cond, msg)                                           \
    do {                                                                       \
      if (!(cond)) {                                                           \
        ::util::detail::assert_fail(#cond, __FILE__, __LINE__, msg);           \
      }                                                                        \
    } while (false)

#  define ASSERT_GET_MACRO(_1, _2, NAME, ...) NAME
#  define ASSERT(...)                                                          \
    ASSERT_GET_MACRO(__VA_ARGS__, ASSERT_IMPL_MSG, ASSERT_IMPL)(__VA_ARGS__)

// -- Debug-only assertions (only enabled in debug builds) ---------------------

#  ifdef LIB_NET_DEBUG

/// @brief Debug-only assertion that aborts if condition is false.
/// Only checked in debug builds. Supports optional message like ASSERT.
#    define DEBUG_ONLY_ASSERT(...) ASSERT(__VA_ARGS__)

#  endif // LIB_NET_DEBUG

#endif // LIB_NET_ENABLE_ASSERTIONS

// -- No-op fallbacks when assertions disabled --------------------------------

#ifndef ASSERT
/// @brief No-op assertion when LIB_NET_ENABLE_ASSERTIONS is not defined
#  define ASSERT(...) (void) 0
#endif

#ifndef DEBUG_ONLY_ASSERT
/// @brief No-op debug assertion when assertions are disabled
#  define DEBUG_ONLY_ASSERT(...) (void) 0
#endif
