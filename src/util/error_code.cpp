/**
 *  @author    Jakob Otto
 *  @file      error_code.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "util/error_code.hpp"

namespace util {

std::string to_string(error_code err) {
  switch (err) {
    case error_code::no_error:
      return "no_error";
    case error_code::runtime_error:
      return "runtime_error";
    case error_code::socket_operation_failed:
      return "socket_operation_failed";
    case error_code::invalid_argument:
      return "invalid_argument";
    case error_code::parser_error:
      return "parser_error";
    case error_code::openssl_error:
      return "openssl_error";
    default:
      return "???";
  }
}

} // namespace util
