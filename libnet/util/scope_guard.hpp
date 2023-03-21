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

template <class Func>
class scope_guard {
public:
  constexpr explicit scope_guard(Func func) : func_(std::move(func)) {
    // nop
  }

  ~scope_guard() {
    if (engaged_)
      func_();
  }

  void reset() { engaged_ = false; }

private:
  Func func_;
  bool engaged_ = true;
};

} // namespace util
