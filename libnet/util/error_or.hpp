/**
 *  @author    Jakob Otto
 *  @file      error_or.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
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
