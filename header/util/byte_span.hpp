/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <cstddef>
#include <span>

namespace util {

using byte_span = std::span<std::byte>;
using const_byte_span = std::span<const std::byte>;

template <class T>
constexpr byte_span as_bytes(T& container) noexcept {
  return {reinterpret_cast<std::byte*>(container.data()), container.size()};
}

template <class T>
constexpr const_byte_span as_const_bytes(const T& container) noexcept {
  return {reinterpret_cast<const std::byte*>(container.data()),
          container.size()};
}

} // namespace util
