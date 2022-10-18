/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace meta {

// -- basic constraints --------------------------------------------------------

/// Concept for checking that type is integral
template <class T>
concept integral = std::is_integral_v<T>;

/// Concept for checking that type is floating point
template <class T>
concept floating = std::is_floating_point_v<T>;

/// Concept for checking that type is an enumeration
template <class T>
concept enumeration = std::is_enum_v<T>;

/// Constraints a template to `T`s derived from `Base`
template <class Base, class T>
concept derived_from = std::is_base_of_v<T, Base>;

/// Constraints a template to `T`s derived from `Base`
template <class T, class U>
concept same_as = std::is_same_v<T, U>;

/// Constraints a template to `T`s derived from `Base`
template <class T, class U>
concept derived_or_same_as = derived_from<U, T> || same_as<T, U>;

// -- compositions of constraints ----------------------------------------------

// Concept that requires types that are trivially serializable
template <class T>
concept trivially_serializable = integral<T> || enumeration<T>;

// Concept that requires types that are not trivially serializable
template <class T>
concept not_trivially_serializable = (!trivially_serializable<T>);

/// Constrains a template to simple (eg. flat types) that allow simple sizeof
/// operations
template <class T>
concept flat_type = integral<T> || floating<T> || enumeration<T>;

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
