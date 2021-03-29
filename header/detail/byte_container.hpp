/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 02.03.2021
 */

#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace detail {

using byte_buffer = std::vector<std::byte>;

template <size_t Size>
using byte_array = std::array<std::byte, Size>;

using byte_span = std::span<std::byte>;

template <class... Ts>
std::array<std::byte, sizeof...(Ts)> make_byte_array(Ts&&... xs) noexcept {
  return {std::byte(std::forward<Ts>(xs))...};
}

template <class Container>
std::span<typename Container::value_type> make_span(Container& container) {
  return std::span(container.data(), container.size());
}

} // namespace detail
