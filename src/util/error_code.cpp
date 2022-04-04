/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "util/error_code.hpp"

namespace util {

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
    case parser_error:
      return "parser_error";
    case openssl_error:
      return "openssl_error";
    default:
      return "???";
  }
}

} // namespace util
