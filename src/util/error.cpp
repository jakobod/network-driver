/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "util/error.hpp"

#include <cerrno>
#include <cstring>
#include <sstream>

namespace util {

error::error(error_code err, std::string err_msg)
  : err_(err), err_msg_(std::move(err_msg)) {
  // nop
}

error::error(error_code err) : err_(err) {
  // nop
}

// -- boolean operators ------------------------------------------------------

bool error::is_error() const {
  return err_ != error_code::no_error;
}

error::operator bool() const {
  return is_error();
}

bool error::operator==(const error& other) {
  return err_ == other.err_;
}

bool error::operator!=(const error& other) {
  return err_ != other.err_;
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
