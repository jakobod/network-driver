/**
 *  @author    Jakob Otto
 *  @file      serialized_size.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "meta/concepts.hpp"

#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <tuple>
#include <utility>

namespace util {

/// @brief Calculates the binary serialized size of C++ objects.
/// Determines how many bytes would be needed to serialize objects using
/// binary_serializer, accounting for all nested structures, containers,
/// and special types like strings and tuples.
class serialized_size {
public:
  /// @brief Default constructor.
  serialized_size() = default;

  /// @brief Static convenience method to calculate serialized size.
  /// @tparam Ts Types of the values to calculate size for.
  /// @param ts The values to measure.
  /// @return The total serialized size in bytes.
  template <class... Ts>
  static std::size_t calculate(const Ts&... ts) {
    return serialized_size{}(ts...);
  }

  /// @brief Function call operator to calculate serialized size.
  /// @tparam Ts Types of the values to calculate size for.
  /// @param ts The values to measure.
  /// @return The total serialized size in bytes.
  template <class... Ts>
  std::size_t operator()(const Ts&... ts) {
    if constexpr (sizeof...(Ts) > 0) {
      return (calculate_size(ts) + ...);
    } else {
      return 0;
    }
  }

private:
  /// @brief Calculates size of a trivial/flat type.
  /// @tparam T A flat type (integral, struct with trivial layout).
  /// @return The size of the type (sizeof(T)).
  template <meta::flat_type T>
  static constexpr std::size_t calculate_size(const T&) noexcept {
    return sizeof(T);
  }

  /// @brief Calculates size of a string.
  /// Accounts for the size prefix (sizeof(std::size_t)) plus string length.
  /// @param str The string to measure.
  /// @return The serialized size.
  static std::size_t calculate_size(const std::string& str) noexcept {
    return sizeof(std::size_t) + str.size();
  }

  /// @brief Calculates size of a pair.
  /// @tparam T Type of first element.
  /// @tparam U Type of second element.
  /// @param p The pair to measure.
  /// @return The combined serialized size.
  template <class T, class U>
  std::size_t calculate_size(const std::pair<T, U>& p) {
    return calculate_size(p.first) + calculate_size(p.second);
  }

  /// @brief Calculates size of a tuple.
  /// @tparam Ts Types of tuple elements.
  /// @param t The tuple to measure.
  /// @return The total serialized size.
  template <class... Ts>
  std::size_t calculate_size(const std::tuple<Ts...>& t) {
    return calculate(std::get<Ts>(t)...);
  }

  /// @brief Calculates size of a fixed-size array.
  /// @tparam T Element type.
  /// @tparam Size The number of elements.
  /// @param arr The array to measure.
  /// @return The serialized size.
  template <class T, size_t Size>
  std::size_t calculate_size(const T (&arr)[Size]) noexcept {
    return calculate_size(arr, Size);
  }

  /// @brief Calculates size of a visitable type.
  /// Delegates to the type's visit() method for custom sizing.
  /// @tparam T A visitable type.
  /// @param t The object to measure.
  /// @return The serialized size.
  template <meta::visitable T>
  std::size_t calculate_size(const T& t) {
    return const_cast<T&>(t).visit(*this);
  }

  // -- Container functions ----------------------------------------------------

  /// @brief Calculates size of a container.
  /// @tparam T Container type with data() and size() methods.
  /// @param container The container to measure.
  /// @return The serialized size including element count.
  template <meta::container T>
  std::size_t calculate_size(const T& container) {
    return calculate_size(container.data(), container.size());
  }

  /// @brief Calculates size of an array of trivial types.
  /// Accounts for size prefix plus raw binary data.
  /// @tparam T A trivial/flat element type.
  /// @param ptr Pointer to the array.
  /// @param size The number of elements.
  /// @return The serialized size.
  template <meta::flat_type T>
  constexpr std::size_t calculate_size(const T*, std::size_t size) {
    return sizeof(std::size_t) + (size * sizeof(T));
  }

  /// @brief Calculates size of an array of complex/visitable types.
  /// Sums the serialized size of each element.
  /// @tparam T A complex/visitable element type.
  /// @param ptr Pointer to the array.
  /// @param size The number of elements.
  /// @return The serialized size.
  template <meta::complex_type T>
  std::size_t calculate_size(const T* ptr, std::size_t size) {
    auto num_bytes = sizeof(std::size_t);
    for (const auto& val : std::span(ptr, size)) {
      num_bytes += calculate_size(val);
    }
    return num_bytes;
  }
};

} // namespace util
