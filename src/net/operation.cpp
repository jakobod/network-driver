/**
 *  @author    Jakob Otto
 *  @file      operation.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/operation.hpp"

#include <format>
#include <numeric>
#include <string>
#include <vector>

namespace net {

std::string to_string(operation op) {
  auto contains = [](operation mask, operation what) -> bool {
    return (mask & what) == what;
  };
  if (op == operation::none) {
    return "operation::[none]";
  }
  std::vector<std::string> parts;
  if (contains(op, operation::read)) {
    parts.emplace_back("read");
  }
  if (contains(op, operation::write)) {
    parts.emplace_back("write");
  }
  if (contains(op, operation::poll_read)) {
    parts.emplace_back("poll_read");
  }
  if (contains(op, operation::poll_write)) {
    parts.emplace_back("poll_write");
  }
  if (contains(op, operation::accept)) {
    parts.emplace_back("accept");
  }

  const auto result = std::accumulate(
    std::next(parts.begin()), parts.end(), parts[0],
    [&](const std::string& a, const std::string& b) { return a + '|' + b; });
  return std::format("operation::[{}]", result);
}

std::ostream& operator<<(std::ostream& os, operation op) {
  return os << to_string(op);
}

} // namespace net
