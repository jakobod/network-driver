/**
 *  @author    Jakob Otto
 *  @file      receive_policy.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <cstdint>

namespace net {

/// @brief Policy specifying minimum and maximum bytes to receive.
/// Defines the receive expectations for a read operation in stream transports.
/// Min size triggers consumption once reached, max size limits buffer size.
struct receive_policy {
  /// @brief Minimum bytes before triggering consumption.
  std::uint32_t min_size;
  /// @brief Maximum bytes to allocate/receive in one buffer.
  std::uint32_t max_size;

  /// @brief Default equality comparison.
  friend bool
  operator==(const receive_policy& lhs, const receive_policy& rhs) noexcept
    = default;

  /// @brief Creates a policy with both min and max constraints.
  /// @param min_size The minimum bytes to accumulate before consumption.
  /// @param max_size The maximum buffer size.
  /// @return A configured receive_policy.
  static constexpr receive_policy between(std::uint32_t min_size,
                                          std::uint32_t max_size) {
    return {min_size, max_size};
  }

  /// @brief Creates a policy expecting exactly a certain number of bytes.
  /// @param size The exact number of bytes expected.
  /// @return A receive_policy with min and max both set to size.
  static constexpr receive_policy exactly(std::uint32_t size) {
    return {size, size};
  }

  /// @brief Creates a policy expecting up to a maximum number of bytes.
  /// Triggers consumption as soon as data is available.
  /// @param max_size The maximum number of bytes to accept.
  /// @return A receive_policy with min_size=1, max_size as specified.
  static constexpr receive_policy up_to(std::uint32_t max_size) {
    return {1, max_size};
  }

  /// @brief Creates a policy that stops receiving.
  /// Used to temporarily pause read operations.
  /// @return A receive_policy with both sizes set to 0.
  static constexpr receive_policy stop() { return {0, 0}; }
};

} // namespace net
