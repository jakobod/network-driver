/**
 *  @author    Jakob Otto
 *  @file      byte_span.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
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
constexpr byte_span as_bytes(T* ptr, std::size_t size) noexcept {
  return {reinterpret_cast<std::byte*>(ptr), size};
}

template <class T>
constexpr const_byte_span as_const_bytes(const T& container) noexcept {
  return {reinterpret_cast<const std::byte*>(container.data()),
          container.size()};
}

template <class T>
constexpr const_byte_span as_const_bytes(T* ptr, std::size_t size) noexcept {
  return {reinterpret_cast<const std::byte*>(ptr), size};
}

} // namespace util
