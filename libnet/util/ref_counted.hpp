/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <atomic>

namespace util {

struct ref_counted {
  virtual ~ref_counted() = default;

  void ref() { ref_count_.fetch_add(1, std::memory_order_relaxed); }

  void deref() {
    if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1)
      delete this;
  }

  std::size_t ref_count() { return ref_count_.load(); }

private:
  std::atomic_size_t ref_count_{1};
};

} // namespace util
