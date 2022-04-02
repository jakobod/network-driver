/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include "meta/type_traits.hpp"

#include <regex>

namespace {

template <std::size_t i>
std::string format_helper(std::string form, std::string arg) {
  using namespace std::string_literals;
  return std::regex_replace(form,
                            std::regex{"\\{"s + std::to_string(i) + "\\}"s},
                            arg);
}

template <std::size_t i, class T,
          std::enable_if_t<std::is_integral_v<T>>* = nullptr>
std::string format_helper(std::string form, T arg) {
  return format_helper<i>(std::move(form), std::to_string(arg));
}

template <std::size_t i, class T>
std::string unpack(std::string form, T arg) {
  return format_helper<i>(std::move(form), std::move(arg));
}

template <std::size_t i, class T, class... Ts>
std::string unpack(std::string form, T arg, Ts... args) {
  auto res = format_helper<i>(std::move(form), std::move(arg));
  return unpack<i + 1>(std::move(res), std::move(args)...);
}

} // namespace

namespace util {

template <class... Ts>
std::string format(std::string form, Ts... args) {
  return unpack<0>(std::move(form), std::move(args)...);
}

std::vector<std::string> split(std::string str, const std::string delim);

std::vector<std::string> split(std::string str, const char delim);

} // namespace util
