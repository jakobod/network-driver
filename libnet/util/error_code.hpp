/**
 *  @author    Jakob Otto
 *  @file      error_code.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <cstdint>
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
