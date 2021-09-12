/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/error_code.hpp"

namespace net {

std::string to_string(error_code err) {
  switch (err) {
    case no_error:
      return "no_error";
    case runtime_error:
      return "runtime_error";
    case socket_operation_failed:
      return "socket_operation_failed";
    case invalid_argument:
      return "invalid_argument";
    default:
      return "???";
  }
}

} // namespace net
