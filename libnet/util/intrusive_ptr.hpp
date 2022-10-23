/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "meta/concepts.hpp"

#include "util/ref_counted.hpp"

namespace util {

template <meta::derived_from<util::ref_counted> T>
class intrusive_ptr {
public:
  // -- Constructors -----------------------------------------------------------

  constexpr intrusive_ptr() noexcept : ptr_(nullptr) {
    // nop
  }

  constexpr intrusive_ptr(std::nullptr_t) noexcept : intrusive_ptr() {
    // nop
  }

  intrusive_ptr(T* raw_ptr, bool add_ref = true) noexcept {
    set_ptr(raw_ptr, add_ref);
  }

  intrusive_ptr(intrusive_ptr&& other) noexcept : ptr_(other.release()) {
    // nop
  }

  intrusive_ptr(const intrusive_ptr& other) noexcept {
    set_ptr(other.get(), true);
  }

  template <meta::convertible_to<T> U>
  intrusive_ptr(intrusive_ptr<U> other) noexcept : ptr_(other.release()) {
    // nop
  }

  ~intrusive_ptr() {
    if (ptr_)
      ptr_->deref();
  }

  // -- Modifiers --------------------------------------------------------------

  void reset(T* new_ptr = nullptr, bool add_ref = true) noexcept {
    if (ptr_)
      ptr_->deref();
    set_ptr(new_ptr, add_ref);
  }

  void swap(intrusive_ptr& other) noexcept { std::swap(ptr_, other.ptr_); }

  T* release() noexcept {
    auto result = ptr_;
    if (ptr_)
      ptr_ = nullptr;
    return result;
  }

  intrusive_ptr& operator=(T* ptr) noexcept {
    reset(ptr);
    return *this;
  }

  intrusive_ptr& operator=(intrusive_ptr&& other) noexcept {
    swap(other);
    return *this;
  }

  intrusive_ptr& operator=(const intrusive_ptr& other) noexcept {
    reset(other.ptr_);
    return *this;
  }

  // -- Observers --------------------------------------------------------------

  T* get() const noexcept { return ptr_; }

  T& operator*() const noexcept { return *ptr_; }

  T* operator->() const noexcept { return ptr_; }

  explicit operator bool() const noexcept { return ptr_ != nullptr; }

private:
  void set_ptr(T* raw_ptr, bool add_ref) noexcept {
    ptr_ = raw_ptr;
    if (ptr_ && add_ref)
      ptr_->ref();
  }

  T* ptr_;
};

// -- Convenience functions to create intrusive_ptrs ---------------------------

template <meta::derived_from<util::ref_counted> T, class... Ts>
intrusive_ptr<T> make_intrusive(Ts&&... ts) {
  return intrusive_ptr<T>(new T(std::forward<Ts>(ts)...), false);
}

template <meta::derived_from<util::ref_counted> T>
intrusive_ptr<T> make_intrusive(T* raw_ptr, bool add_ref = true) {
  return intrusive_ptr<T>(raw_ptr, add_ref);
}

// -- Raw pointer comparisons --------------------------------------------------

template <meta::derived_from<util::ref_counted> T>
bool operator==(const intrusive_ptr<T>& a, const T* b) {
  return a.get() == b;
}

template <meta::derived_from<util::ref_counted> T>
bool operator==(const T* a, const intrusive_ptr<T>& b) {
  return a == b.get();
}

template <meta::derived_from<util::ref_counted> T>
bool operator!=(const intrusive_ptr<T>& a, const T* b) {
  return a.get() != b;
}

template <meta::derived_from<util::ref_counted> T>
bool operator!=(const T* a, const intrusive_ptr<T>& b) {
  return a != b.get();
}

// -- nullptr comparisons ------------------------------------------------------

template <meta::derived_from<util::ref_counted> T>
bool operator==(const intrusive_ptr<T>& ptr, std::nullptr_t) {
  return !ptr;
}

template <meta::derived_from<util::ref_counted> T>
bool operator==(std::nullptr_t, const intrusive_ptr<T>& ptr) {
  return !ptr;
}

template <meta::derived_from<util::ref_counted> T>
bool operator!=(const intrusive_ptr<T>& ptr, std::nullptr_t) {
  return static_cast<bool>(ptr);
}

template <meta::derived_from<util::ref_counted> T>
bool operator!=(std::nullptr_t, const intrusive_ptr<T>& ptr) {
  return static_cast<bool>(ptr);
}

} // namespace util
