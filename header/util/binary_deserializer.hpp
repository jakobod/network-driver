/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include "meta/type_traits.hpp"

#include <cstring>
#include <numeric>
#include <variant>

namespace util {

class binary_deserializer {
public:
  explicit binary_deserializer(const_byte_span bytes);

  template <class... Ts>
  void operator()(Ts&... ts) {
    (deserialize(ts), ...);
  }

private:
  template <class T,
            std::enable_if_t<meta::is_trivially_serializable_v<T>>* = nullptr>
  void deserialize(T& val) {
    if (bytes_.size() < sizeof(T))
      throw std::runtime_error("Not enough bytes to deserialize T");
    std::memcpy(&val, bytes_.data(), sizeof(T));
    bytes_ = bytes_.subspan(sizeof(T));
  }

  void deserialize(float& val);

  void deserialize(double& val);

  template <class T, class U>
  void deserialize(std::pair<T, U>& p) {
    deserialize(p.first);
    deserialize(p.second);
  }

  template <class... Ts>
  void deserialize(std::tuple<Ts...>& t) {
    (deserialize(std::get<Ts>(t)), ...);
  }

  template <class T, std::enable_if_t<meta::is_visitable_v<T>>* = nullptr>
  void deserialize(T& t) {
    t.visit(*this);
  }

  template <class T, std::enable_if_t<meta::is_container_v<T>>* = nullptr>
  void deserialize(T& container) {
    std::size_t size;
    deserialize(size);
    if constexpr (meta::has_resize_member_v<T>)
      container.resize(size);
    if (container.size() < size)
      throw std::runtime_error("Container does not have enough free space");
    deserialize(container.data(), size);
  }

  // -- range deserialize functions --------------------------------------------

  // Serializes integral types
  template <class T,
            std::enable_if_t<meta::is_trivially_serializable_v<T>>* = nullptr>
  void deserialize(T* ptr, std::size_t size) {
    const auto num_bytes = size * sizeof(T);
    if (bytes_.size() < num_bytes)
      throw std::runtime_error("Not enough bytes to deserialize T");
    std::memcpy(ptr, bytes_.data(), num_bytes);
    bytes_ = bytes_.subspan(num_bytes);
  }

  // Serializes visitable types
  template <class T,
            std::enable_if_t<!meta::is_trivially_serializable_v<T>>* = nullptr>
  void deserialize(T* ptr, std::size_t size) {
    for (auto& val : make_span(ptr, size))
      deserialize(val);
  }

  const_byte_span bytes_;
};

} // namespace util
