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

/// Concept for checking that type is integral
template <class T>
concept integral = std::is_integral_v<T>;

/// Concept for checking that type is floating point
template <class T>
concept floating = std::is_floating_point_v<T>;

/// Concept for checking that type is an enumeration
template <class T>
concept enumeration = std::is_enum_v<T>;

/// Concept for checking that type is a pointer
template <class T>
concept pointer = std::is_pointer_v<T>;

template <class To, class From>
concept convertible_to = std::is_convertible_v<To*, From*>;

/// Constrains a template to `T`s derived from `Base`
template <class Base, class T>
concept derived_from = std::is_base_of_v<T, Base>;

/// Constrains a template to `T`s that are equal to `U`.
template <class T, class U>
concept same_as = std::is_same_v<T, U>;

/// Constrains a template to `U` that is equal to one of `Ts`
template <class U, class... Ts>
concept one_of = (std::is_same_v<Ts, U> || ...);

// -- compositions of Constraints ----------------------------------------------

// Concept that requires types that are trivially serializable
template <class T>
concept trivially_serializable = integral<T> || enumeration<T> || pointer<T>
                                 || std::is_null_pointer_v<T>;

// Concept that requires types that are not trivially serializable
template <class T>
concept not_trivially_serializable = (!trivially_serializable<T>);

/// Constrains a template to simple (eg. flat types) that allow simple sizeof
/// operations
template <class T>
concept flat_type = integral<T> || floating<T> || enumeration<T> || pointer<T>
                    || std::is_null_pointer_v<T>;

/// Constrains a template to complex (e.g. non-flat types) types such as
/// integral, floating points, or enums
template <class T>
concept complex_type = (!flat_type<T>);

// -- Concepts -----------------------------------------------------------------

/// Constrains a template to resizable types
template <class T>
concept resizable = requires(T t) { t.resize(std::declval<std::size_t>()); };

/// Constrains a template to container types
template <class T>
concept container = requires(T t) {
                      t.begin();
                      t.end();
                      t.data();
                      t.size();
                    };

// -- Visitable concept --------------------------------------------------------

struct visitable_helper {
  template <class T, class... Ts>
  auto operator()(const T&, const Ts&...) {}
};

template <class T>
concept visitable
  = requires(T t) { t.visit(std::declval<visitable_helper&>()); };

} // namespace meta
