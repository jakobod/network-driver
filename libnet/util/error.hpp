/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/fwd.hpp"

#include "util/error_code.hpp"

#include <ostream>
#include <string>

namespace util {

class [[nodiscard]] error {
public:
  error(error_code err, std::string err_msg);

  error(error_code err);

  error() = default;

  ~error() = default;

  // -- boolean operators ------------------------------------------------------

  bool is_error() const;

  operator bool() const;

  bool operator==(const error& other);

  bool operator!=(const error& other);

  friend std::ostream& operator<<(std::ostream& os, const error& err) {
    return os << to_string(err.err_) << std::string(": ") << err.err_msg_;
  }

private:
  error_code err_ = no_error;
  std::string err_msg_;
};

/// Error-constant for returning no-error.
const error none{};

/// Stringifies an error
std::string to_string(const error& err);

/// Reads errno and returns the according error string for it
std::string last_error_as_string();

} // namespace util
