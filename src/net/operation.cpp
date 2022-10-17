/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
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

} // namespace net
