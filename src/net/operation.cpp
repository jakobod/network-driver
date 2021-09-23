/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/operation.hpp"

#include <bitset>
#include <sstream>

namespace net {

std::string to_string(operation op) {
  auto contains = [](operation mask, operation what) -> bool {
    return (mask & what) == what;
  };
  if (op == operation::none)
    return "[ none ]";
  std::stringstream ss;
  ss << "[";
  if (contains(op, operation::read))
    ss << " read ";
  if (contains(op, operation::write))
    ss << " write ";
  if (contains(op, operation::edge_triggered))
    ss << " edge_triggered ";
  if (contains(op, operation::exclusive))
    ss << " exclusive ";
  if (contains(op, operation::one_shot))
    ss << " one_shot ";
  ss << "]";
  return ss.str();
}

} // namespace net
