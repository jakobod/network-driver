/**
 *  @author    Jakob Otto
 *  @file      byte_array.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace util {

/// @brief Type alias for a fixed-size array of bytes.
/// @tparam Size The number of bytes in the array.
template <size_t Size>
using byte_array = std::array<std::byte, Size>;

/// @brief Creates a byte array from integral values.
/// Constructs a byte_array with the specified byte values.
/// @tparam Ts Types of the values to convert (must be integral).
/// @param args The values to include in the byte array.
/// @return A byte_array with the converted values.
template <class... Ts>
constexpr byte_array<sizeof...(Ts)> make_byte_array(Ts&&... args) noexcept {
  return {std::byte(std::forward<Ts>(args))...};
}

} // namespace util
