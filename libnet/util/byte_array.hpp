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

template <size_t Size>
using byte_array = std::array<std::byte, Size>;

template <class... Ts>
constexpr util::byte_array<sizeof...(Ts)>
make_byte_array(Ts&&... args) noexcept {
  return {std::byte(std::forward<Ts>(args))...};
}

} // namespace util
