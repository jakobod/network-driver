/**
 *  @author    Jakob Otto
 *  @file      ref_counted.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <atomic>

namespace util {

/// @brief Base class for objects with intrusive reference counting.
/// Provides atomic reference counting suitable for use with intrusive_ptr.
/// Derived classes should be managed exclusively through intrusive_ptr
/// and must not be copied or moved.
struct ref_counted {
  // -- Constructors -----------------------------------------------------------

  /// @brief Default constructor initializes reference count to 1.
  ref_counted() = default;

  /// @brief Virtual destructor for proper cleanup in derived classes.
  virtual ~ref_counted() = default;

  /// @brief Copy construction is deleted.
  ref_counted(const ref_counted&) = delete;
  /// @brief Copy assignment is deleted.
  ref_counted& operator=(const ref_counted&) = delete;
  /// @brief Move construction is deleted.
  ref_counted(ref_counted&&) noexcept = delete;
  /// @brief Move assignment is deleted.
  ref_counted& operator=(ref_counted&&) noexcept = delete;

  // -- Ref counting interface -------------------------------------------------

  /// @brief Increments the reference count by one.
  /// Call this when acquiring a reference to the object.
  void ref() noexcept { ref_count_.fetch_add(1, std::memory_order_relaxed); }

  /// @brief Decrements the reference count and potentially deletes the object.
  /// When the reference count drops to zero, the object is deleted via
  /// its virtual destructor.
  void deref() {
    if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      delete this;
    }
  }

  /// @brief Retrieves the current reference count.
  /// @return The number of active references to this object.
  std::size_t ref_count() const noexcept { return ref_count_.load(); }

private:
  /// @brief The atomic reference counter, initially 1.
  std::atomic_size_t ref_count_{1};
};

} // namespace util
