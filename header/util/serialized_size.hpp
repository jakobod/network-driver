/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include "meta/type_traits.hpp"

#include <cstddef>
#include <cstring>
#include <string>
#include <type_traits>

namespace util {

class serialized_size {
public:
  serialized_size() = default;

  template <class T, class... Ts>
  static std::size_t calculate(const T& what, const Ts&... ts) {
    return serialized_size{}(what, ts...);
  }

  template <class T, class... Ts>
  std::size_t operator()(const T& what, const Ts&... ts) {
    return calculate_size(what) + this->operator()(ts...);
  }

  template <class T>
  std::size_t operator()(const T& what) {
    return calculate_size(what);
  }

private:
  template <class T, std::enable_if_t<std::is_integral_v<T>>* = nullptr>
  static constexpr std::size_t calculate_size(const T&) noexcept {
    return sizeof(T);
  }

  static constexpr std::size_t calculate_size(const std::byte&) noexcept {
    return sizeof(std::byte);
  }

  static std::size_t calculate_size(const std::string& str) noexcept {
    return sizeof(std::size_t) + str.size();
  }

  template <class T, std::enable_if_t<meta::is_visitable_v<T>>* = nullptr>
  constexpr std::size_t calculate_size(const T& t) noexcept {
    return visit(t, *this);
  }

  // -- Container functions ----------------------------------------------------

  template <class T, std::enable_if<meta::has_data_member_v<T>>* = nullptr>
  static std::size_t calculate_size(const T& container) {
    return calculate_size(container.data(), container.size());
  }

  // Calculates size for integral types
  template <class T>
  static std::size_t calculate_size(const T*, std::size_t size) {
    return 0;
  }

  // // Calculates size for integral types
  // template <class T, std::enable_if<std::is_integral_v<T>>* = nullptr>
  // static std::size_t calculate_size(const T*, std::size_t size) {
  //   return sizeof(std::size_t) + (size * sizeof(T));
  // }

  // // Calculates size for visitable types
  // template <class T, std::enable_if_t<meta::is_visitable_v<T>>* = nullptr>
  // static std::size_t calculate_size(const T* ptr, std::size_t size) {
  //   auto num_bytes = sizeof(std::size_t);
  //   for (const auto& val : make_span(ptr, size))
  //     num_bytes += visit(val);
  //   return num_bytes;
  // }

  // // Calculates size for visitable types
  // template <class T,
  //           std::enable_if_t<
  //             !std::is_integral_v<T> && !meta::is_visitable_v<T>>* = nullptr>
  // static std::size_t calculate_size(const T* ptr, std::size_t size) {
  //   auto num_bytes = sizeof(std::size_t);
  //   for (const auto& val : make_span(ptr, size))
  //     num_bytes += calculate_size(val);
  //   return num_bytes;
  // }
};

} // namespace util
