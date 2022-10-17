/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
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
