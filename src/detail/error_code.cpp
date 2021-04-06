/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 05.04.2021
 */

#include "detail/error_code.hpp"

namespace detail {

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

} // namespace detail
