/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <sstream>
#include <string>
#include <variant>

#include "fwd.hpp"
#include "net/error_code.hpp"

namespace net {

struct error {
  error(error_code err, std::string err_msg);

  error(error_code err);

  error();

  ~error() = default;

  // -- boolean operators ------------------------------------------------------

  operator bool() const;

  bool operator==(const error& other);

  bool operator!=(const error& other);

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

template <class T>
T& get(error_or<T>& maybe_error) {
  return std::get<T>(maybe_error);
}

std::string to_string(const error& err);

} // namespace net
