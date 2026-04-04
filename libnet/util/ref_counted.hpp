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

struct ref_counted {
  // -- Constructors -----------------------------------------------------------

  /// @brief Constructor
  ref_counted() = default;

  /// @brief Destructor
  virtual ~ref_counted() = default;

  /// @brief Deleted copy construction
  ref_counted(const ref_counted&) = delete;
  /// @brief Deleted copy assignment
  ref_counted& operator=(const ref_counted&) = delete;
  /// @brief Deleted copy construction
  ref_counted(ref_counted&&) noexcept = delete;
  /// @brief Deleted move assignment
  ref_counted& operator=(ref_counted&&) noexcept = delete;

  // -- Ref counting interface -------------------------------------------------

  /// @brief Increments the reference count by 1
  void ref() noexcept { ref_count_.fetch_add(1, std::memory_order_relaxed); }

  /// @brief Decrements the reference count by 1
  /// @details Will delete the referenced instance if the ref count drops to 0
  void deref() {
    if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      delete this;
    }
  }

  /// @brief Retrieves the current reference count of the instance
  /// @return The current reference_count
  std::size_t ref_count() const noexcept { return ref_count_.load(); }

private:
  /// @brief The reference count
  std::atomic_size_t ref_count_{1};
};

} // namespace util
