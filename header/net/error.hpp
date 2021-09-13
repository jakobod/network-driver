/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <string>
#include <variant>

#include "fwd.hpp"
#include "net/error_code.hpp"

namespace net {

struct error {
  error(error_code err, std::string err_msg)
    : err_(err), err_msg_(std::move(err_msg)) {
    // nop
  }

  error(error_code err) : err_(err), err_msg_("") {
    // nop
  }

  error() : err_(no_error), err_msg_("") {
    // nop
  }

  ~error() = default;

  // -- boolean operators ------------------------------------------------------

  operator bool() const {
    return err_ != no_error;
  }

  bool operator==(const error& other) {
    return err_ == other.err_;
  }

  bool operator!=(const error& other) {
    return err_ != other.err_;
  }

  friend std::ostream& operator<<(std::ostream& os, const error& err) {
    return os << to_string(err.err_) << std::string(": ") << err.err_msg_;
  }

private:
  error_code err_;
  std::string err_msg_;
};

const error none{};

template <class T>
error* get_error(error_or<T>& maybe_error) {
  return std::get_if<error>(&maybe_error);
}

} // namespace net
