/**
 *  @author    Jakob Otto
 *  @file      error.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "util/error.hpp"

#include <cerrno>
#include <cstring>
#include <sstream>

namespace util {

error::error(error_code code, std::string msg)
  : code_(code), msg_(std::move(msg)) {
  // nop
}

error::error(error_code code) : code_(code) {
  // nop
}

// -- boolean operators ------------------------------------------------------

bool error::is_error() const noexcept {
  return code() != error_code::no_error;
}

error::operator bool() const noexcept {
  return is_error();
}

bool error::operator==(const error& other) const noexcept {
  return code() == other.code();
}

bool error::operator!=(const error& other) const noexcept {
  return code() != other.code();
}

std::string to_string(const error& err) {
  std::stringstream ss;
  ss << err;
  return ss.str();
}

std::string last_error_as_string() {
  return std::strerror(errno);
}

} // namespace util
