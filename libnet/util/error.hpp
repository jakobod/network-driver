/**
 *  @author    Jakob Otto
 *  @file      error.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "util/error_code.hpp"

#include <format>
#include <ostream>
#include <string>

namespace util {

class [[nodiscard]] error {
public:
  error(error_code code) : code_(code) {
    // nop
  }

  error(error_code code, std::string msg) : code_{code}, msg_{std::move(msg)} {
    // nop
  }

  template <class... Arguments>
  error(error_code code, std::format_string<Arguments...> form,
        Arguments... args)
    : error{code, std::format(form, std::move(args)...)} {
    // nop
  }

  error() = default;

  ~error() = default;

  // -- accessors --------------------------------------------------------------

  error_code code() const noexcept { return code_; }

  const std::string& msg() const { return msg_; }

  // -- boolean operators ------------------------------------------------------

  bool is_error() const noexcept;

  operator bool() const noexcept;

  bool operator==(const error& other) const noexcept;

  bool operator!=(const error& other) const noexcept;

  friend std::ostream& operator<<(std::ostream& os, const error& err) {
    return os << to_string(err.code()) << std::string(": ") << err.msg();
  }

private:
  error_code code_{error_code::no_error};
  std::string msg_{};
};

/// Error-constant for returning no-error.
static const error none{};

/// Stringifies an error
std::string to_string(const error& err);

/// Reads errno and returns the according error string for it
std::string last_error_as_string();

} // namespace util
