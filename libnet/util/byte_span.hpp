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

/// @brief Type alias for a mutable span of bytes.
/// Provides a non-owning view over a sequence of bytes.
using byte_span = std::span<std::byte>;

/// @brief Type alias for a const span of bytes.
/// Provides a read-only view over a sequence of bytes.
using const_byte_span = std::span<const std::byte>;

/// @brief Converts a container to a mutable byte span.
/// Interprets the container's data as a sequence of bytes.
/// @tparam T The container type (must have data() and size()).
/// @param container The container to view as bytes.
/// @return A byte_span over the container's data.
template <class T>
constexpr byte_span as_bytes(T& container) noexcept {
  return {reinterpret_cast<std::byte*>(container.data()), container.size()};
}

/// @brief Converts a pointer and size to a mutable byte span.
/// Interprets the pointed-to memory as a sequence of bytes.
/// @tparam T The type of the pointed-to objects.
/// @param ptr The pointer to the data.
/// @param size The number of elements (not bytes).
/// @return A byte_span over the memory region.
template <class T>
constexpr byte_span as_bytes(T* ptr, std::size_t size) noexcept {
  return {reinterpret_cast<std::byte*>(ptr), size};
}

/// @brief Converts a const container to a const byte span.
/// Interprets the container's data as a read-only sequence of bytes.
/// @tparam T The container type (must have data() and size()).
/// @param container The container to view as bytes.
/// @return A const_byte_span over the container's data.
template <class T>
constexpr const_byte_span as_const_bytes(const T& container) noexcept {
  return {reinterpret_cast<const std::byte*>(container.data()),
          container.size()};
}

/// @brief Converts a const pointer and size to a const byte span.
/// Interprets the pointed-to memory as a read-only sequence of bytes.
/// @tparam T The type of the pointed-to objects.
/// @param ptr The const pointer to the data.
/// @param size The number of elements (not bytes).
/// @return A const_byte_span over the memory region.
template <class T>
constexpr const_byte_span as_const_bytes(T* ptr, std::size_t size) noexcept {
  return {reinterpret_cast<const std::byte*>(ptr), size};
}

} // namespace util
