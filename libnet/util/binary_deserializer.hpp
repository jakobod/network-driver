/**
 *  @author    Jakob Otto
 *  @file      binary_deserializer.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "meta/concepts.hpp"

#include "util/byte_span.hpp"

#include <cstring>
#include <numeric>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace util {

/// @brief Deserializes C++ objects from binary byte sequences.
/// Supports the inverse of binary_serializer: trivially serializable types,
/// std::pair, std::tuple, custom visitable types, arrays, and containers.
/// Throws std::runtime_error if not enough bytes are available.
class binary_deserializer {
public:
  /// @brief Constructs a deserializer from binary data.
  /// @param bytes The byte span containing serialized data.
  explicit binary_deserializer(const_byte_span bytes);

  /// @brief Deserializes one or more values from the byte stream.
  /// @tparam Ts Types of the values to deserialize.
  /// @param ts The lvalue references to store deserialized values in.
  /// @throws std::runtime_error If there are not enough bytes.
  template <class... Ts>
  void operator()(Ts&... ts) {
    (deserialize(ts), ...);
  }

private:
  /// @brief Deserializes a trivially serializable type (memcpy).
  /// @tparam T A type with standard layout and no non-trivial operations.
  /// @param val The variable to store the deserialized value in.
  /// @throws std::runtime_error If fewer than sizeof(T) bytes remain.
  template <meta::trivially_serializable T>
  void deserialize(T& val) {
    if (bytes_.size() < sizeof(T)) {
      throw std::runtime_error("Not enough bytes to deserialize T");
    }
    std::memcpy(&val, bytes_.data(), sizeof(T));
    bytes_ = bytes_.subspan(sizeof(T));
  }

  /// @brief Specialization for deserializing float values.
  /// @param val The variable to store the deserialized float in.
  /// @throws std::runtime_error If not enough bytes remain.
  void deserialize(float& val);

  /// @brief Specialization for deserializing double values.
  /// @param val The variable to store the deserialized double in.
  /// @throws std::runtime_error If not enough bytes remain.
  void deserialize(double& val);

  /// @brief Deserializes a pair of values.
  /// @tparam T The type of the first element.
  /// @tparam U The type of the second element.
  /// @param p The pair to populate with deserialized values.
  template <class T, class U>
  void deserialize(std::pair<T, U>& p) {
    deserialize(p.first);
    deserialize(p.second);
  }

  /// @brief Deserializes a tuple of values.
  /// @tparam Ts The types of the tuple elements.
  /// @param t The tuple to populate with deserialized values.
  template <class... Ts>
  void deserialize(std::tuple<Ts...>& t) {
    (deserialize(std::get<Ts>(t)), ...);
  }

  /// @brief Deserializes a visitable type by delegating to its visit() method.
  /// @tparam T A type that provides a visit() method accepting a deserializer.
  /// @param t The visitable object to populate.
  template <meta::visitable T>
  void deserialize(T& t) {
    t.visit(*this);
  }

  /// @brief Deserializes a container (vector, string, etc.).
  /// Reads the element count and resizes the container if needed.
  /// @tparam T A container type with data(), size(), and optionally resize().
  /// @param container The container to populate.
  /// @throws std::runtime_error If container is too small or not resizable.
  template <meta::container T>
  void deserialize(T& container) {
    std::size_t size;
    deserialize(size);
    if constexpr (meta::resizable<T>) {
      container.resize(size);
    }
    if (container.size() < size) {
      throw std::runtime_error("Container does not have enough free space");
    }
    deserialize(container.data(), size);
  }

  // -- range deserialize functions --------------------------------------------

  /// @brief Deserializes a range of trivially serializable values.
  /// @tparam T A trivially serializable element type.
  /// @param ptr Pointer to the array to populate.
  /// @param size The number of elements to deserialize.
  /// @throws std::runtime_error If not enough bytes remain.
  template <meta::trivially_serializable T>
  void deserialize(T* ptr, std::size_t size) {
    const auto num_bytes = size * sizeof(T);
    if (bytes_.size() < num_bytes) {
      throw std::runtime_error("Not enough bytes to deserialize T");
    }
    std::memcpy(ptr, bytes_.data(), num_bytes);
    bytes_ = bytes_.subspan(num_bytes);
  }

  /// @brief Deserializes a range of non-trivial (visitable) values.
  /// @tparam T A non-trivially serializable element type.
  /// @param ptr Pointer to the array to populate.
  /// @param size The number of elements to deserialize.
  /// @throws std::runtime_error If not enough bytes remain.
  template <meta::not_trivially_serializable T>
  void deserialize(T* ptr, std::size_t size) {
    for (auto& val : make_span(ptr, size)) {
      deserialize(val);
    }
  }

  const_byte_span bytes_; ///< The input byte stream.
};

} // namespace util
