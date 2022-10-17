/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 18.02.2021
 */

#pragma once

#include <string>

namespace util {

enum class error_code : std::uint8_t {
  no_error = 0,
  runtime_error,
  socket_operation_failed,
  invalid_argument,
  parser_error,
  openssl_error,
};

std::string to_string(error_code err);

} // namespace util
