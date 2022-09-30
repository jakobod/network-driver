/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/fwd.hpp"

#include "meta/type_traits.hpp"

#include <cstddef>
#include <cstring>
#include <span>
#include <string>
#include <tuple>
#include <utility>

namespace util {

class serialized_size {
public:
  serialized_size() = default;

  template <class... Ts>
  static std::size_t calculate(const Ts&... ts) {
    return serialized_size{}(ts...);
  }

  template <class... Ts>
  std::size_t operator()(const Ts&... ts) {
    if constexpr (sizeof...(Ts) > 0)
      return (calculate_size(ts) + ...);
    else
      return 0;
  }

private:
  template <class T, std::enable_if_t<meta::has_trivial_size_v<T>>* = nullptr>
  static constexpr std::size_t calculate_size(const T&) noexcept {
    return sizeof(T);
  }

  static std::size_t calculate_size(const std::string& str) noexcept {
    return sizeof(std::size_t) + str.size();
  }

  template <class T, class U>
  std::size_t calculate_size(const std::pair<T, U>& p) {
    return calculate_size(p.first) + calculate_size(p.second);
  }

  template <class... Ts>
  std::size_t calculate_size(const std::tuple<Ts...>& t) {
    return calculate(std::get<Ts>(t)...);
  }

  template <class T, size_t Size>
  std::size_t calculate_size(const T (&arr)[Size]) noexcept {
    return calculate_size(arr, Size);
  }

  template <class T, std::enable_if_t<meta::is_visitable_v<T>>* = nullptr>
  std::size_t calculate_size(const T& t) {
    return const_cast<T&>(t).visit(*this);
  }

  // -- Container functions ----------------------------------------------------

  template <class T, std::enable_if_t<meta::is_container_v<T>>* = nullptr>
  std::size_t calculate_size(const T& container) {
    return calculate_size(container.data(), container.size());
  }

  // Calculates size for integral types
  template <class T, std::enable_if_t<meta::has_trivial_size_v<T>>* = nullptr>
  constexpr std::size_t calculate_size(const T*, std::size_t size) {
    return sizeof(std::size_t) + (size * sizeof(T));
  }

  template <class T, std::enable_if_t<!meta::has_trivial_size_v<T>>* = nullptr>
  std::size_t calculate_size(const T* ptr, std::size_t size) {
    auto num_bytes = sizeof(std::size_t);
    for (const auto& val : std::span(ptr, size))
      num_bytes += calculate_size(val);
    return num_bytes;
  }
};

} // namespace util
