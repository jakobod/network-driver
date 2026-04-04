/**
 *  @author    Jakob Otto
 *  @file      format.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "meta/concepts.hpp"

#include <charconv>
#include <cstdint>
#include <regex>
#include <string>
#include <string_view>
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

/// @brief Formats a string using positional placeholders and arguments.
/// Supports printf-like formatting using {0}, {1}, etc. placeholders.
/// Integral types are automatically converted to strings.
/// @tparam Ts The types of arguments to format.
/// @param form The format string with {0}, {1}, ... placeholders.
/// @param args The arguments to substitute into placeholders.
/// @return The formatted string.
template <class... Ts>
std::string format(std::string form, Ts... args) {
  return unpack<0>(std::move(form), std::move(args)...);
}

/// @brief Splits a string by a delimiter string.
/// Separates the input string into multiple strings using the delimiter.
/// @param str The string to split.
/// @param delim The delimiter string.
/// @return A vector of substrings.
std::vector<std::string_view> split(std::string_view str,
                                    std::string_view delim);

/// @brief Splits a string by a single character delimiter.
/// Separates the input string into multiple strings using a single character.
/// @param str The string to split.
/// @param delim The character delimiter.
/// @return A vector of substrings.
std::vector<std::string_view> split(std::string_view str, const char delim);

/// @brief Joins multiple strings with a delimiter.
/// Concatenates strings together, separating them by a delimiter character.
/// @param strings The strings to join.
/// @param delim The character to insert between strings.
/// @return The joined string.
std::string join(const std::vector<std::string>& strings, const char delim);

/// @brief Removes all occurrences of a character from a string.
/// @param str The string to process.
/// @param unwanted The character to remove.
/// @return The string with all occurrences of unwanted removed.
std::string remove(std::string str, char unwanted);

/// @brief Parses a string to a value of type T
/// @tparam T The type of the value to parse to
/// @param str The string to parse
/// @param t The value to parse to
/// @return true if successful, false otherwise
template <class T>
bool parse(std::string_view str, T& t) {
  return std::from_chars(str.data(), str.data() + str.size(), t).ec
         == std::errc{};
}

} // namespace util
