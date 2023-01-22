/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <cstdint>
#include <string>

namespace net {

/// Operation abstraction with same definition as epoll.
enum class operation : std::uint32_t {
  none = 0x00,
  read = 0x01,
  write = 0x02,
  read_write = read | write,
};

/// @relates operation
constexpr operation operator|(std::uint32_t x, operation y) {
  return static_cast<operation>(x | static_cast<std::uint32_t>(y));
}

/// @relates operation
constexpr operation operator&(std::uint32_t x, operation y) {
  return static_cast<operation>(x & static_cast<std::uint32_t>(y));
}

/// @relates operation
constexpr operation operator^(std::uint32_t x, operation y) {
  return static_cast<operation>(x ^ static_cast<std::uint32_t>(y));
}

/// @relates operation
constexpr operation operator|(operation x, operation y) {
  return static_cast<operation>(static_cast<std::uint32_t>(x)
                                | static_cast<std::uint32_t>(y));
}

/// @relates operation
constexpr operation operator&(operation x, operation y) {
  return static_cast<operation>(static_cast<std::uint32_t>(x)
                                & static_cast<std::uint32_t>(y));
}

/// @relates operation
constexpr operation operator^(operation x, operation y) {
  return static_cast<operation>(static_cast<std::uint32_t>(x)
                                ^ static_cast<std::uint32_t>(y));
}

/// @relates operation
constexpr operation operator~(operation x) {
  return static_cast<operation>(~static_cast<std::uint32_t>(x));
}

std::string to_string(operation op);

std::ostream& operator<<(std::ostream& os, operation op);

} // namespace net
