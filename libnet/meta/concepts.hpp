/**
 *  @author    Jakob Otto
 *  @file      concepts.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <concepts>
#include <cstdint>
#include <string>
#include <type_traits>

namespace meta {

// -- basic Constraints --------------------------------------------------------

/// @brief Concept: type is an integral type (int, char, etc.).
template <class T>
concept integral = std::is_integral_v<T>;

/// @brief Concept: type is a floating-point type (float, double).
template <class T>
concept floating = std::is_floating_point_v<T>;

/// @brief Concept: type is an enumeration.
template <class T>
concept enumeration = std::is_enum_v<T>;

/// @brief Concept: type is a pointer type.
template <class T>
concept pointer = std::is_pointer_v<T>;

/// @brief Concept: To* is convertible to From*.
template <class To, class From>
concept convertible_to = std::is_convertible_v<To*, From*>;

/// @brief Concept: T is derived from (or is the same as) Base.
template <class Base, class T>
concept derived_from = std::is_base_of_v<T, Base>;

/// @brief Concept: T is the same type as U.
template <class T, class U>
concept same_as = std::is_same_v<T, U>;

/// @brief Concept: U is one of the types Ts.
template <class U, class... Ts>
concept one_of = (std::is_same_v<Ts, U> || ...);

// -- compositions of Constraints ----------------------------------------------

/// @brief Concept: type can be trivially serialized using memcpy.
/// Includes integral types, enumerations, pointers, and nullptr.
template <class T>
concept trivially_serializable = integral<T> || enumeration<T> || pointer<T>
                                 || std::is_null_pointer_v<T>;

/// @brief Concept: type cannot be trivially serialized (inverse of
/// trivially_serializable).
template <class T>
concept not_trivially_serializable = (!trivially_serializable<T>);

/// @brief Concept: simple/flat type suitable for sizeof operations.
/// Includes integrals, floating-point, enums, and pointers with no indirection.
template <class T>
concept flat_type = integral<T> || floating<T> || enumeration<T> || pointer<T>
                    || std::is_null_pointer_v<T>;

/// @brief Concept: complex (non-flat) type requiring custom serialization.
template <class T>
concept complex_type = (!flat_type<T>);

// -- Concepts -----------------------------------------------------------------

/// @brief Concept: type supports resize() method.
/// Indicates a container that can be resized to given capacity.
template <class T>
concept resizable = requires(T t) { t.resize(std::declval<std::size_t>()); };

/// @brief Concept: type is a container.
/// Must support begin(), end(), data(), and size() methods.
template <class T>
concept container = requires(T t) {
  t.begin();
  t.end();
  t.data();
  t.size();
};

// -- Visitable concept --------------------------------------------------------

/// @brief Helper type for visitable concept detection.
struct visitable_helper {
  /// @brief Dummy template for matching visit() calls.
  template <class T, class... Ts>
  auto operator()(const T&, const Ts&...) {}
};

/// @brief Concept: type is visitable (supports visit() method).
/// Types that provide a visit() method for custom operations like
/// serialization, deserialization, or introspection.
template <class T>
concept visitable
  = requires(T t) { t.visit(std::declval<visitable_helper&>()); };

} // namespace meta
