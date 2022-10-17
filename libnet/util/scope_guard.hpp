/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
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
