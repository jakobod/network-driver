/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 06.04.2021
 */

#include "net/operation.hpp"

namespace net {

std::string to_string(operation op) {
  switch (op) {
    case operation::read:
      return "read";
    case operation::write:
      return "write";
    default:
      return "unknown operation";
  }
}

} // namespace net
