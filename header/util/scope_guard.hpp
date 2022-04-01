/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <functional>

namespace util {

template <class Func>
class scope_guard {
public:
  explicit scope_guard(Func func) : func_(std::move(func)) {
    // nop
  }

  ~scope_guard() {
    if (!disabled_)
      func_();
  }

  void reset() {
    disabled_ = true;
  }

private:
  Func func_;
  bool disabled_ = false;
};

} // namespace util
