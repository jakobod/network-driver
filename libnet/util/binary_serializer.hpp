/**
 *  @author    Jakob Otto
 *  @file      binary_serializer.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "util/serialized_size.hpp"

#include "meta/concepts.hpp"

#include "util/byte_buffer.hpp"
#include "util/byte_span.hpp"

#include <cstring>
#include <tuple>
#include <utility>

namespace util {

/// @brief Serializes C++ objects into binary byte buffers.
/// Supports trivially serializable types, std::pair, std::tuple, custom
/// visitable types, arrays, and containers. Automatically allocates buffer
/// space as needed.
class binary_serializer {
public:
  /// @brief Constructs a serializer that writes to the provided buffer.
  /// @param buf The byte buffer to write serialized data to.
  explicit binary_serializer(util::byte_buffer& buf);

  /// @brief Serializes one or more values into the buffer.
  /// Automatically resizes the buffer if necessary.
  /// @tparam Ts Types of the values to serialize.
  /// @param ts The values to serialize.
  template <class... Ts>
  void operator()(const Ts&... ts) {
    realloc(serialized_size{}(ts...));
    (serialize(ts), ...);
  }

private:
  /// @brief Reallocates buffer if needed to ensure enough free space.
  /// @param required_free_space The amount of space required (in bytes).
  void realloc(std::size_t required_free_space);

  /// @brief Serializes a trivially serializable type (memcpy).
  /// @tparam T A type with standard layout and no non-trivial operations.
  /// @param i The value to serialize.
  template <meta::trivially_serializable T>
  void serialize(const T& i) {
    std::memcpy(free_space_.data(), &i, sizeof(T));
    free_space_ = free_space_.subspan(sizeof(T));
  }

  /// @brief Specialization for serializing float values.
  /// @param val The float value to serialize.
  void serialize(const float& val);

  /// @brief Specialization for serializing double values.
  /// @param val The double value to serialize.
  void serialize(const double& val);

  /// @brief Serializes a pair of values.
  /// @tparam T The type of the first element.
  /// @tparam U The type of the second element.
  /// @param p The pair to serialize.
  template <class T, class U>
  void serialize(const std::pair<T, U>& p) {
    serialize(p.first);
    serialize(p.second);
  }

  /// @brief Serializes a tuple of values.
  /// @tparam Ts The types of the tuple elements.
  /// @param t The tuple to serialize.
  template <class... Ts>
  void serialize(const std::tuple<Ts...>& t) {
    (serialize(std::get<Ts>(t)), ...);
  }

  /// @brief Serializes a visitable type by delegating to its visit() method.
  /// @tparam T A type that provides a visit() method accepting a serializer.
  /// @param what The visitable object to serialize.
  template <meta::visitable T>
  void serialize(const T& what) {
    const_cast<T&>(what).visit(*this);
  }

  /// @brief Serializes a fixed-size array.
  /// @tparam T The element type.
  /// @tparam Size The number of elements.
  /// @param arr The array to serialize.
  template <class T, size_t Size>
  void serialize(const T (&arr)[Size]) noexcept {
    serialize(arr, Size);
  }

  /// @brief Serializes a container (vector, string, etc.).
  /// @tparam T A container type with data() and size() methods.
  /// @param container The container to serialize.
  template <meta::container T>
  void serialize(const T& container) {
    serialize(container.data(), container.size());
  }

  // -- range serialize functions ----------------------------------------------

  /// @brief Serializes a range of trivially serializable values.
  /// Writes the count followed by the raw binary data.
  /// @tparam T A trivially serializable element type.
  /// @param ptr Pointer to the array data.
  /// @param size The number of elements.
  template <meta::trivially_serializable T>
  void serialize(const T* ptr, std::size_t size) {
    serialize(size);
    const auto num_bytes = size * sizeof(T);
    std::memcpy(free_space_.data(), ptr, num_bytes);
    free_space_ = free_space_.subspan(num_bytes);
  }

  /// @brief Serializes a range of non-trivial (visitable) values.
  /// Writes the count followed by serialized elements.
  /// @tparam T A non-trivially serializable element type.
  /// @param ptr Pointer to the array data.
  /// @param size The number of elements.
  template <meta::not_trivially_serializable T>
  void serialize(const T* ptr, std::size_t size) {
    serialize(size);
    for (const auto& val : make_span(ptr, size)) {
      serialize(val);
    }
  }

  byte_buffer& buf_;     ///< Reference to the output buffer.
  byte_span free_space_; ///< View of the free space in the buffer.
};

} // namespace util
