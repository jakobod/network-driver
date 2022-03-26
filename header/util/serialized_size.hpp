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
  serialized_size() = delete;

  template <class T, class... Ts>
  static constexpr std::size_t calculate(const T& what, const Ts&... ts) {
    return calculate(what) + calculate(ts...);
  }

  template <class T>
  static constexpr std::size_t calculate(const T& what) {
    return calculate_size(what);
  }

private:
  template <class T, std::enable_if_t<std::is_integral_v<T>>* = nullptr>
  static constexpr std::size_t calculate_size(const T&) {
    return sizeof(T);
  }

  // TODO
  // template <class C, std::enable_if_t<meta::is_container_v<C>>* = nullptr>
  // void serialize([[maybe_unused]] const C& container) {
  //   serialize(container.size());
  //   for (const auto& val : container)
  //     serialize(val);
  // }

  static std::size_t calculate_size(const std::string& str) {
    return sizeof(std::size_t) + str.size();
  }

  static std::size_t calculate_size(const std::byte&) {
    return sizeof(std::byte);
  }
};

} // namespace util
