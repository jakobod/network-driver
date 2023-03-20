/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/fwd.hpp"

#include "meta/concepts.hpp"

#include <cstdint>
#include <regex>
#include <string>
#include <vector>

namespace {

template <std::size_t i>
std::string format_helper(std::string form, std::string arg) {
  using namespace std::string_literals;
  return std::regex_replace(form,
                            std::regex{"\\{"s + std::to_string(i) + "\\}"s},
                            arg);
}

template <std::size_t i, meta::integral T>
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

std::vector<std::string> split(std::string str, const std::string& delim);

std::vector<std::string> split(const std::string& str, const char delim);

std::string join(const std::vector<std::string>& strings, const char delim);

std::string remove(std::string str, char unwanted);

} // namespace util
