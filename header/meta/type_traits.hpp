/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <type_traits>

namespace {

// clang-format off
#define member_trait(trait_name, member)                                       \
  template <class T, class = void>                                             \
  struct trait_name : std::false_type {};                                      \
  template <class T>                                                           \
  struct trait_name<T, decltype(std::declval<T>().member)> : std::true_type {};\
  template <class T>                                                           \
  constexpr bool trait_name##_v = trait_name<T>::value
// clang-format on

} // namespace

namespace meta {

// -- Visitable trait ----------------------------------------------------------

struct visitor {
  template <class T, class... Ts>
  auto operator()(const T&, const Ts&...) {
    // nop
  }
};

template <class T, class = void>
struct is_visitable : std::false_type {};

template <class T>
struct is_visitable<T, decltype(visit(std::declval<T>(),
                                      std::declval<visitor&>()))>
  : std::true_type {};

template <class T>
constexpr bool is_visitable_v = is_visitable<T>::value;

// -- Data-member trait --------------------------------------------------------

template <class T, class = void>
struct has_data_member : std::false_type {};

template <class T>
struct has_data_member<T, decltype(std::declval<T>().data(), void())>
  : std::true_type {};

template <class T>
constexpr bool has_data_member_v = has_data_member<T>::value;

// -- Size-member trait --------------------------------------------------------

template <class T, class = void>
struct has_size_member : std::false_type {};

template <class T>
struct has_size_member<T, decltype(std::declval<T>().size(), void())>
  : std::true_type {};

template <class T>
constexpr bool has_size_member_v = has_size_member<T>::value;

// -- Container trait ----------------------------------------------------------

template <class T>
constexpr bool is_container_v = has_data_member_v<T>&& has_size_member_v<T>;

} // namespace meta
