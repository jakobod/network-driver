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

template <class T, class Visitor, class = void>
struct is_visitable : std::false_type {};

template <class T, class Visitor>
struct is_visitable<
  T, Visitor, decltype(visit(std::declval<T>(), std::declval<Visitor&>()))>
  : std::true_type {};

template <class T, class Visitor>
constexpr bool is_visitable_v = is_visitable<T, Visitor>::value;

// -- Data-member trait --------------------------------------------------------
member_trait(has_data_member, data());

// -- Size-member trait --------------------------------------------------------
member_trait(has_size_member, size());

// -- Size-member trait --------------------------------------------------------
member_trait(has_begin_member, begin());

// -- Size-member trait --------------------------------------------------------
member_trait(has_end_member, end());

// -- Container trait ----------------------------------------------------------
// clang-format off
template <class T>
constexpr bool is_container_v = has_data_member_v<T> && 
                                has_size_member_v<T> &&
                                has_begin_member_v<T> && 
                                has_end_member_v<T>;
// clang-format on

} // namespace meta
