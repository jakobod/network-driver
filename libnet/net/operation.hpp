/**
 *  @author    Jakob Otto
 *  @file      operation.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <cstdint>
#include <string>

namespace net {

/// Operation abstraction with same definition as epoll.
enum class operation : std::uint8_t {
  none = 0x00,
  read = 0x01,
  write = 0x02,
  read_write = read | write,
  accept = 0x04,
};

/// @relates operation
constexpr operation operator|(std::uint8_t x, operation y) {
  return static_cast<operation>(x | static_cast<std::uint8_t>(y));
}

/// @relates operation
constexpr operation operator&(std::uint8_t x, operation y) {
  return static_cast<operation>(x & static_cast<std::uint8_t>(y));
}

/// @relates operation
constexpr operation operator^(std::uint8_t x, operation y) {
  return static_cast<operation>(x ^ static_cast<std::uint8_t>(y));
}

/// @relates operation
constexpr operation operator|(operation x, operation y) {
  return static_cast<operation>(static_cast<std::uint8_t>(x)
                                | static_cast<std::uint8_t>(y));
}

/// @relates operation
constexpr operation operator&(operation x, operation y) {
  return static_cast<operation>(static_cast<std::uint8_t>(x)
                                & static_cast<std::uint8_t>(y));
}

/// @relates operation
constexpr operation operator^(operation x, operation y) {
  return static_cast<operation>(static_cast<std::uint8_t>(x)
                                ^ static_cast<std::uint8_t>(y));
}

/// @relates operation
constexpr operation operator~(operation x) {
  return static_cast<operation>(~static_cast<std::uint8_t>(x));
}

std::string to_string(operation op);

std::ostream& operator<<(std::ostream& os, operation op);

} // namespace net
