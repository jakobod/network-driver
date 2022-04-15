/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/fwd.hpp"

#include "util/error_code.hpp"

#include <sstream>
#include <string>
#include <variant>

namespace util {

class [[nodiscard]] error {
public:
  error(error_code err, std::string err_msg);

  error(error_code err);

  error();

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
  error_code err_;
  std::string err_msg_;
};

const error none{};

std::string to_string(const error& err);

} // namespace util
