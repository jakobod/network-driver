/**
 *  @author    Jakob Otto
 *  @file      assert.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#if defined(LIB_NET_ENABLE_ASSERTIONS)

#  include <sstream>

#  ifndef ABORT_FUNCTION
#    include <cstdlib>
#    define ABORT_FUNCTION std::abort
#  endif

#  ifndef ASSERTION_OUTPUT_STREAM
#    include <iostream>
#    define ASSERTION_OUTPUT_STREAM std::cerr
#  endif

// -- Assertion implementation -------------------------------------------------

namespace util::detail {

/// @brief Stream-based assertion handler for piping messages.
/// Collects streamed data and triggers assertion on destruction.
class AssertionStream {
public:
  /// @brief Constructs an assertion stream.
  /// @param condition The stringified condition
  /// @param file The source file
  /// @param line The line number
  /// @param failed Whether the assertion condition failed
  AssertionStream(const char* condition, const char* file, int line,
                  bool failed) noexcept
    : condition_{condition}, file_{file}, line_{line}, failed_{failed} {}

  /// @brief Destructor triggers the assertion if it failed.
  /// noexcept specification depends on whether ABORT_FUNCTION is noexcept.
  ~AssertionStream() noexcept(noexcept(ABORT_FUNCTION())) {
    if (failed_) {
      ASSERTION_OUTPUT_STREAM << "\n=== ASSERTION FAILED ===\n"
                              << "File: " << file_ << ":" << line_ << "\n"
                              << "Condition: " << condition_ << "\n";
      if (!message_.str().empty()) {
        ASSERTION_OUTPUT_STREAM << "Message: " << message_.str() << "\n";
      }
      ASSERTION_OUTPUT_STREAM << "======================\n" << std::endl;
      ABORT_FUNCTION();
    }
  }

  /// @brief Stream operator for piping messages
  template <typename T>
  AssertionStream& operator<<(const T& value) noexcept {
    if (failed_) {
      message_ << value;
    }
    return *this;
  }

private:
  const char* condition_;
  const char* file_;
  int line_;
  bool failed_;
  std::ostringstream message_;
};

} // namespace util::detail

// -- Release assertions (always enabled) --------------------------------------

/// @brief Runtime assertion that supports streaming messages.
/// Aborts the program if the condition is false.
/// Can stream messages using <<: ASSERT(cond) << "message"
/// @param cond The condition to check
#  define ASSERT(cond)                                                         \
    ::util::detail::AssertionStream(#cond, __FILE__, __LINE__, !(cond))

// -- Debug-only assertions (only enabled in debug builds) ---------------------

#  ifdef LIB_NET_DEBUG

/// @brief Debug-only assertion that supports streaming messages.
/// Only checked in debug builds. Aborts the program if the condition is false.
/// Can stream messages using <<: DEBUG_ONLY_ASSERT(cond) << "message"
/// @param cond The condition to check
#    define DEBUG_ONLY_ASSERT(cond) ASSERT(cond)

#  endif // LIB_NET_DEBUG

#endif // LIB_NET_ENABLE_ASSERTIONS

// -- Anything that has not been defined shall be default defined
// ---------------

#ifndef ASSERT
/// @brief No-op assertion for when assertions are disabled
#  define ASSERT(...)
#endif

#ifndef DEBUG_ONLY_ASSERT
/// @brief No-op debug assertion for when assertions are disabled
#  define DEBUG_ONLY_ASSERT(...)
#endif
