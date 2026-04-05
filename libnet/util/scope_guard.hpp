/**
 *  @author    Jakob Otto
 *  @file      scope_guard.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <utility>

namespace util {

/// @brief RAII-style scope guard that executes a function on scope exit.
/// Executes a callable when the scope_guard is destroyed, useful for
/// cleanup operations and resource management.
/// @tparam Func The type of the callable to execute (typically a lambda).
template <class Func>
class scope_guard {
public:
  /// @brief Constructs a scope_guard with a function to execute on exit.
  /// @param func The callable to invoke when the guard is destroyed.
  constexpr explicit scope_guard(Func func) : func_(std::move(func)) {
    // nop
  }

  /// @brief Destructor invokes the function if still engaged.
  /// The function is executed when the scope_guard goes out of scope,
  /// unless reset() was called.
  ~scope_guard() {
    if (engaged_) {
      func_();
    }
  }

  /// @brief Disengages the guard, preventing function execution on exit.
  /// Useful when the cleanup operation should be skipped.
  void reset() { engaged_ = false; }

private:
  Func func_;           ///< The callable to invoke.
  bool engaged_ = true; ///< Whether the guard is engaged.
};

} // namespace util
