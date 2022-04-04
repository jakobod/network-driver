/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "util/error.hpp"

namespace util {

error::error(error_code err, std::string err_msg)
  : err_(err), err_msg_(std::move(err_msg)) {
  // nop
}

error::error(error_code err) : err_(err), err_msg_("") {
  // nop
}

error::error() : err_(no_error), err_msg_("") {
  // nop
}

// -- boolean operators ------------------------------------------------------

error::operator bool() const {
  return err_ != no_error;
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

} // namespace util
