/**
 *  @author    Jakob Otto
 *  @file      intrusive_ptr.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "meta/concepts.hpp"

#include "util/ref_counted.hpp"

namespace util {

/// @brief Smart pointer for reference-counted objects using intrusive counting.
/// Manages ownership of objects derived from ref_counted, automatically
/// incrementing/decrementing the object's internal reference count.
/// The reference counting is intrusive, meaning the count is stored in
/// the managed object itself.
/// @tparam T The type of the managed object (must derive from ref_counted).
template <meta::derived_from<util::ref_counted> T>
class intrusive_ptr {
public:
  // -- Constructors -----------------------------------------------------------

  /// @brief Default constructor creates a null pointer.
  constexpr intrusive_ptr() noexcept = default;

  /// @brief Constructs a null pointer from nullptr literal.
  /// @param Null pointer literal.
  constexpr intrusive_ptr(std::nullptr_t) noexcept : intrusive_ptr() {
    // nop
  }

  /// @brief Constructs from a raw pointer.
  /// Optionally increments the reference count of the object.
  /// @param raw_ptr The raw pointer to manage.
  /// @param add_ref Whether to increment the reference count (default: true).
  intrusive_ptr(T* raw_ptr, bool add_ref = true) noexcept {
    set_ptr(raw_ptr, add_ref);
  }

  /// @brief Move constructor transfers ownership.
  /// @param other The intrusive_ptr to move from.
  intrusive_ptr(intrusive_ptr&& other) noexcept : ptr_(other.release()) {
    // nop
  }

  /// @brief Move constructor from convertible intrusive_ptr.
  /// @tparam U A type convertible to T*.
  /// @param other The intrusive_ptr to move from.
  template <meta::convertible_to<T> U>
  intrusive_ptr(intrusive_ptr<U>&& other) noexcept : ptr_(other.release()) {
    // nop
  }

  /// @brief Copy constructor duplicates ownership.
  /// Increments the reference count of the shared object.
  /// @param other The intrusive_ptr to copy from.
  intrusive_ptr(const intrusive_ptr& other) noexcept {
    set_ptr(other.get(), true);
  }

  /// @brief Copy constructor from convertible intrusive_ptr.
  /// @tparam U A type convertible to T*.
  /// @param other The intrusive_ptr to copy from.
  template <meta::convertible_to<T> U>
  intrusive_ptr(const intrusive_ptr<U>& other) noexcept {
    set_ptr(other.get(), true);
  }

  /// @brief Destructor releases ownership.
  /// Decrements the reference count and deletes the object if count drops to
  /// zero.
  ~intrusive_ptr() {
    if (ptr_) {
      ptr_->deref();
    }
  }

  // -- Modifiers --------------------------------------------------------------

  /// @brief Resets the pointer to a new value.
  /// Releases the previous object and adopts a new one.
  /// @param new_ptr The new raw pointer to manage.
  /// @param add_ref Whether to increment the reference count (default: true).
  void reset(T* new_ptr = nullptr, bool add_ref = true) noexcept {
    if (ptr_) {
      ptr_->deref();
    }
    set_ptr(new_ptr, add_ref);
  }

  /// @brief Swaps pointers with another intrusive_ptr.
  /// @param other The other intrusive_ptr to swap with.
  void swap(intrusive_ptr& other) noexcept { std::swap(ptr_, other.ptr_); }

  /// @brief Releases ownership without decreasing the reference count.
  /// Returns the raw pointer and sets this to nullptr.
  /// @return The previously managed pointer.
  T* release() noexcept {
    auto result = ptr_;
    if (ptr_) {
      ptr_ = nullptr;
    }
    return result;
  }

  /// @brief Move assignment from raw pointer.
  /// @param ptr The raw pointer to assign.
  /// @return Reference to this.
  intrusive_ptr& operator=(T* ptr) noexcept {
    reset(ptr);
    return *this;
  }

  /// @brief Move assignment operator.
  /// @param other The intrusive_ptr to move from.
  /// @return Reference to this.
  intrusive_ptr& operator=(intrusive_ptr&& other) noexcept {
    swap(other);
    return *this;
  }

  /// @brief Copy assignment operator.
  /// @param other The intrusive_ptr to copy from.
  /// @return Reference to this.
  intrusive_ptr& operator=(const intrusive_ptr& other) noexcept {
    reset(other.ptr_);
    return *this;
  }

  // -- Observers --------------------------------------------------------------

  /// @brief Gets the raw pointer.
  /// @return The managed pointer, or nullptr if empty.
  T* get() const noexcept { return ptr_; }

  /// @brief Dereferences the pointer.
  /// @return A reference to the managed object.
  T& operator*() const noexcept { return *ptr_; }

  /// @brief Arrow operator for member access.
  /// @return The managed pointer.
  T* operator->() const noexcept { return ptr_; }

  /// @brief Boolean conversion.
  /// @return True if the pointer is not null.
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

private:
  void set_ptr(T* raw_ptr, bool add_ref) noexcept {
    ptr_ = raw_ptr;
    if (ptr_ && add_ref) {
      ptr_->ref();
    }
  }

  T* ptr_{nullptr};
};

// -- Convenience functions to create intrusive_ptrs ---------------------------

/// @brief Creates an intrusive_ptr for a new object.
/// Constructs the object with the provided arguments and wraps it in a pointer.
/// @tparam T The ref_counted-derived type to construct.
/// @tparam Ts Types of constructor arguments.
/// @param ts Arguments to pass to T's constructor.
/// @return An intrusive_ptr to the new object.
template <meta::derived_from<util::ref_counted> T, class... Ts>
intrusive_ptr<T> make_intrusive(Ts&&... ts) {
  return intrusive_ptr<T>(new T(std::forward<Ts>(ts)...), false);
}

/// @brief Wraps a raw pointer in an intrusive_ptr.
/// @tparam T The ref_counted-derived type.
/// @param raw_ptr The raw pointer to wrap.
/// @param add_ref Whether to increment the reference count (default: true).
/// @return An intrusive_ptr to the object.
template <meta::derived_from<util::ref_counted> T>
intrusive_ptr<T> make_intrusive(T* raw_ptr, bool add_ref = true) {
  return intrusive_ptr<T>(raw_ptr, add_ref);
}

template <meta::derived_from<util::ref_counted> T>
intrusive_ptr<T> as_intrusive_ptr(T& ref, bool add_ref = true) {
  return intrusive_ptr<T>(&ref, add_ref);
}

// -- Raw pointer comparisons --------------------------------------------------

/// @brief Equality comparison with raw pointer.
/// @relates intrusive_ptr
/// @param a The intrusive_ptr.
/// @param b The raw pointer.
/// @return True if the intrusive_ptr manages the same object as b.
template <meta::derived_from<util::ref_counted> T>
bool operator==(const util::intrusive_ptr<T>& a, const T* b) {
  return a.get() == b;
}

/// @brief Equality comparison with raw pointer (reversed operands).
/// @relates intrusive_ptr
/// @param a The raw pointer.
/// @param b The intrusive_ptr.
/// @return True if a points to the same object as the intrusive_ptr.
template <meta::derived_from<util::ref_counted> T>
bool operator==(const T* a, const util::intrusive_ptr<T>& b) {
  return a == b.get();
}

/// @brief Inequality comparison with raw pointer.
/// @relates intrusive_ptr
/// @param a The intrusive_ptr.
/// @param b The raw pointer.
/// @return True if they point to different objects.
template <meta::derived_from<util::ref_counted> T>
bool operator!=(const util::intrusive_ptr<T>& a, const T* b) {
  return a.get() != b;
}

/// @brief Inequality comparison with raw pointer (reversed operands).
/// @relates intrusive_ptr
/// @param a The raw pointer.
/// @param b The intrusive_ptr.
/// @return True if they point to different objects.
template <meta::derived_from<util::ref_counted> T>
bool operator!=(const T* a, const util::intrusive_ptr<T>& b) {
  return a != b.get();
}

// -- intrusive_ptr-to-intrusive_ptr comparisons -------------------------------

/// @brief Equality comparison between two intrusive_ptrs.
/// @relates intrusive_ptr
/// @param a The first intrusive_ptr.
/// @param b The second intrusive_ptr.
/// @return True if both manage the same object.
template <meta::derived_from<util::ref_counted> T,
          meta::derived_from<util::ref_counted> U>
bool operator==(const util::intrusive_ptr<T>& a,
                const util::intrusive_ptr<U>& b) {
  return a.get() == b.get();
}

/// @brief Inequality comparison between two intrusive_ptrs.
/// @relates intrusive_ptr
/// @param a The first intrusive_ptr.
/// @param b The second intrusive_ptr.
/// @return True if they manage different objects.
template <meta::derived_from<util::ref_counted> T,
          meta::derived_from<util::ref_counted> U>
bool operator!=(const util::intrusive_ptr<T>& a,
                const util::intrusive_ptr<U>& b) {
  return a.get() != b.get();
}

// -- nullptr comparisons ------------------------------------------------------

/// @brief Equality comparison with nullptr.
/// @relates intrusive_ptr
/// @param ptr The intrusive_ptr.
/// @return True if the pointer is null.
template <meta::derived_from<util::ref_counted> T>
constexpr bool
operator==(const util::intrusive_ptr<T>& ptr, std::nullptr_t) noexcept {
  return !ptr;
}

/// @brief Equality comparison with nullptr (reversed operands).
/// @relates intrusive_ptr
/// @param ptr The intrusive_ptr.
/// @return True if the pointer is null.
template <meta::derived_from<util::ref_counted> T>
constexpr bool
operator==(std::nullptr_t, const util::intrusive_ptr<T>& ptr) noexcept {
  return !ptr;
}

/// @brief Inequality comparison with nullptr.
/// @relates intrusive_ptr
/// @param ptr The intrusive_ptr.
/// @return True if the pointer is not null.
template <meta::derived_from<util::ref_counted> T>
constexpr bool
operator!=(const util::intrusive_ptr<T>& ptr, std::nullptr_t) noexcept {
  return static_cast<bool>(ptr);
}

/// @brief Inequality comparison with nullptr (reversed operands).
/// @relates intrusive_ptr
/// @param ptr The intrusive_ptr.
/// @return True if the pointer is not null.
template <meta::derived_from<util::ref_counted> T>
constexpr bool
operator!=(std::nullptr_t, const util::intrusive_ptr<T>& ptr) noexcept {
  return static_cast<bool>(ptr);
}

} // namespace util
