/**
 *  @author    Jakob Otto
 *  @file      operation.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/operation.hpp"
#include "util/format.hpp"

#include <string>
#include <vector>

namespace net {

std::string to_string(operation op) {
  auto contains = [](operation mask, operation what) -> bool {
    return (mask & what) == what;
  };
  if (op == operation::none)
    return "operation::[none]";
  std::vector<std::string> parts;
  if (contains(op, operation::read))
    parts.emplace_back("read");
  if (contains(op, operation::write))
    parts.emplace_back("write");
  return util::format("operation::[{0}]", util::join(parts, '|'));
}

std::ostream& operator<<(std::ostream& os, operation op) {
  return os << to_string(op);
}

} // namespace net
