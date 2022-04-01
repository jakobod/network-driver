/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include "util/serialized_size.hpp"

#include "meta/type_traits.hpp"

#include <cstring>

namespace util {

class binary_serializer {
public:
  explicit binary_serializer(byte_buffer& buf);

  template <class... Ts>
  void operator()(const Ts&... ts) {
    realloc(serialized_size{}(ts...));
    (serialize(ts), ...);
  }

private:
  void realloc(std::size_t required_free_space);

  template <class T,
            std::enable_if_t<meta::is_trivially_serializable_v<T>>* = nullptr>
  void serialize(const T& i) {
    std::memcpy(free_space_.data(), &i, sizeof(T));
    free_space_ = free_space_.subspan(sizeof(T));
  }

  void serialize(const float& val);

  void serialize(const double& val);

  template <class T, class U>
  void serialize(const std::pair<T, U>& p) {
    serialize(p.first);
    serialize(p.second);
  }

  template <class... Ts>
  void serialize(const std::tuple<Ts...>& t) {
    (serialize(std::get<Ts>(t)), ...);
  }

  template <class T, std::enable_if_t<meta::is_visitable_v<T>>* = nullptr>
  void serialize(const T& what) {
    visit(const_cast<T&>(what), *this);
  }

  template <class T, size_t Size>
  void serialize(const T (&arr)[Size]) noexcept {
    serialize(arr, Size);
  }

  template <class T, std::enable_if_t<meta::is_container_v<T>>* = nullptr>
  void serialize(const T& container) {
    serialize(container.data(), container.size());
  }

  // -- range serialize functions ----------------------------------------------

  // Serializes integral types
  template <class T,
            std::enable_if_t<meta::is_trivially_serializable_v<T>>* = nullptr>
  void serialize(const T* ptr, std::size_t size) {
    serialize(size);
    const auto num_bytes = size * sizeof(T);
    std::memcpy(free_space_.data(), ptr, num_bytes);
    free_space_ = free_space_.subspan(num_bytes);
  }

  // Serializes visitable types
  template <class T,
            std::enable_if_t<!meta::is_trivially_serializable_v<T>>* = nullptr>
  void serialize(const T* ptr, std::size_t size) {
    serialize(size);
    const auto num_bytes = size * sizeof(T);
    for (const auto& val : make_span(ptr, size))
      serialize(val);
  }

  byte_buffer& buf_;
  byte_span free_space_;
};

} // namespace util
