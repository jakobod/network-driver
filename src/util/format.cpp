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

namespace util {

std::vector<std::string> split(std::string str, const std::string& delim) {
  std::vector<std::string> tokens;
  size_t pos = 0;
  while ((pos = str.find(delim)) != std::string::npos) {
    tokens.emplace_back(str.substr(0, pos));
    str.erase(0, pos + delim.length());
  }
  tokens.emplace_back(std::move(str));
  return tokens;
}

std::vector<std::string> split(const std::string& str, const char delim) {
  std::vector<std::string> tokens;
  std::stringstream stream(str);
  std::string token;
  while (std::getline(stream, token, delim))
    tokens.emplace_back(std::move(token));
  return tokens;
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
