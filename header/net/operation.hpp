/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 03.03.2021
 */

#pragma once

#include <string>

namespace net {

/// Operation abstraction with same definition as epoll.
enum class operation : uint16_t {
  none = 0x000,
  read = 0x001,
  write = 0x004,
  read_write = 0x001 | 0x004,
};

/// @relates operation
constexpr operation operator|(operation x, operation y) {
  return static_cast<operation>(static_cast<uint16_t>(x)
                                | static_cast<uint16_t>(y));
}

/// @relates operation
constexpr operation operator&(operation x, operation y) {
  return static_cast<operation>(static_cast<uint16_t>(x)
                                & static_cast<uint16_t>(y));
}

/// @relates operation
constexpr operation operator^(operation x, operation y) {
  return static_cast<operation>(static_cast<uint16_t>(x)
                                ^ static_cast<uint16_t>(y));
}

/// @relates operation
constexpr operation operator~(operation x) {
  return static_cast<operation>(~static_cast<uint16_t>(x));
}

std::string to_string(operation op) {
  switch (op) {
    case operation::read:
      return "read";
    case operation::write:
      return "write";
    default:
      return "unknown operation";
  }
}

} // namespace net
