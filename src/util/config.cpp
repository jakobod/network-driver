/**
 *  @author    Jakob Otto
 *  @file      config.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "util/config.hpp"

#include "util/format.hpp"

#include <algorithm>
#include <fstream>
#include <stack>
#include <string_view>

using dictionary = util::config::dictionary;

namespace {

/// @brief Checks wether the string encodes a boolean value
/// @param value  The string to check
/// @returns true in case the string encodes a boolean value, false otherwise
bool is_bool(const std::string_view value) {
  return (value == "true") || (value == "false");
}

/// @brief Checks wether the value encodes a double value
/// @param value  The string to check
/// @returns true in case the string encodes a double value, false otherwise
bool is_double(const std::string_view value) {
  if (std::count(value.begin(), value.end(), '.') != 1) {
    return false;
  }
  return std::all_of(value.begin(), value.end(), [](const char c) {
    return (((c >= '0') && (c <= '9')) || (c == '.'));
  });
}

/// @brief Checks wether the value encodes an integer value
/// @param value  The string to check
/// @returns true in case the string encodes an integer value, false otherwise
bool is_integer(const std::string_view value) {
  if (std::count(value.begin(), value.end(), '.') != 0) {
    return false;
  }
  return std::all_of(value.begin(), value.end(),
                     [](const char c) { return ((c >= '0') && (c <= '9')); });
}

/// @brief Parses a given line from a config file and adds it to the dictionary
/// @param prefix  The current prefix of the option
/// @param line  The line to parse
/// @param dict  The dictionary to add the parsed entry to
void parse_line(const std::string& prefix, const std::string& line,
                dictionary& dict) {
  const auto parts = util::split(line, '=');
  if ((parts.size() != 2) || parts.front().empty() || parts.back().empty()) {
    throw std::runtime_error{"Error while parsing config line=\"" + line
                             + "\""};
  }

  const auto key = prefix + parts.front();

  if (is_bool(parts.back())) {
    dict.emplace(key, (parts.back() == "true"));
  } else if (is_double(parts.back())) {
    dict.emplace(key, std::stod(parts.back()));
  } else if (is_integer(parts.back())) {
    dict.emplace(key, std::stoll(parts.back()));
  } else {
    dict.emplace(key, parts.back());
  }
}

dictionary parse_file(const std::string& config_path) {
  // Parsing state
  dictionary dict;
  std::ifstream is{config_path};
  std::string line;
  std::string prefix;
  std::stack<std::size_t> prefix_lengths;

  while (std::getline(is, line)) {
    // Remove all space characters and skip the line if it is empty
    line = util::remove(line, ' ');
    if (line.empty()) {
      continue;
    }

    // Manage the scope
    if (line.find('{') != std::string::npos) {
      auto new_scope = util::remove(line, '{') + '.';
      prefix_lengths.push(new_scope.size());
      prefix.append(new_scope);
    } else if (line.find('}') != std::string::npos) {
      prefix.resize(prefix.size() - prefix_lengths.top());
      prefix_lengths.pop();
    } else {
      parse_line(prefix, line, dict);
    }
  }
  return dict;
}

} // namespace

namespace util {

config::config(const std::string& config_path)
  : config_values_{parse_file(config_path)} {
  // nop
}

void config::parse(const std::string& config_path) {
  config_values_ = parse_file(config_path);
}

const dictionary& config::get_entries() const noexcept {
  return config_values_;
}

} // namespace util
