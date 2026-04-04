/**
 *  @author    Jakob Otto
 *  @file      format.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "util/format.hpp"

#include <numeric>
#include <sstream>

namespace {

std::vector<std::string_view> split_impl(std::string_view str, auto delim,
                                         std::size_t delim_size) {
  std::vector<std::string_view> tokens;
  size_t pos = 0;
  while ((pos = str.find(delim)) != std::string::npos) {
    tokens.emplace_back(str.substr(0, pos));
    str = str.substr(pos + delim_size);
  }
  tokens.emplace_back(std::move(str));
  return tokens;
}

} // namespace

namespace util {

std::vector<std::string_view> split(std::string_view str,
                                    std::string_view delim) {
  return split_impl(str, delim, delim.size());
}

std::vector<std::string_view> split(std::string_view str, const char delim) {
  return split_impl(str, delim, 1);
}

std::string join(const std::vector<std::string>& strings, const char delim) {
  return std::accumulate(std::next(strings.begin()), strings.end(),
                         strings.front(),
                         [delim](const std::string& a, const std::string& b) {
                           return a + delim + b;
                         });
}

std::string remove(std::string str, char unwanted) {
  str.erase(std::remove_if(str.begin(), str.end(),
                           [unwanted](const char c) { return c == unwanted; }),
            str.end());
  return str;
}

} // namespace util
