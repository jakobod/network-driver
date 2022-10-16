/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "util/error.hpp"

#include <variant>

namespace util {

template <class T>
using error_or = std::variant<T, error>;

template <class T>
error* get_error(error_or<T>& maybe_error) {
  return std::get_if<error>(&maybe_error);
}

template <class T>
const error* get_error(const error_or<T>& maybe_error) {
  return std::get_if<error>(&maybe_error);
}

} // namespace util
