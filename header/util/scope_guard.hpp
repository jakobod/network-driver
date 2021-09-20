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
  scope_guard(Func func) : func_(std::move(func)) {
    // nop
  }

  ~scope_guard() {
    func_();
  }

private:
  Func func_;
};

} // namespace util
