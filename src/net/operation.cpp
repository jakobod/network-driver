/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/operation.hpp"

#include <sstream>

namespace net {

std::string to_string(operation op) {
  auto contains = [](operation mask, operation what) -> bool {
    return (mask & what) == what;
  };
  if (op == operation::none)
    return "none";
  std::stringstream ss;
  ss << "operation::[";
  if (contains(op, operation::read))
    ss << "read|";
  if (contains(op, operation::write))
    ss << "write|";
  ss << "]";
  return ss.str();
}

} // namespace net
