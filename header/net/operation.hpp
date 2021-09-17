/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <string>
#include <sys/epoll.h>

namespace net {

/// Operation abstraction with same definition as epoll.
enum class operation : uint32_t {
  none = 0x000,
  read = 0x001,
  write = 0x004,
  read_write = 0x001 | 0x004,
  one_shot = EPOLLONESHOT,
  one_shot_read = read | one_shot,
  one_shot_write = write | one_shot,
  one_shot_read_write = read_write | one_shot,
};

/// @relates operation
constexpr operation operator|(uint32_t x, operation y) {
  return static_cast<operation>(x | static_cast<uint32_t>(y));
}

/// @relates operation
constexpr operation operator&(uint32_t x, operation y) {
  return static_cast<operation>(x & static_cast<uint32_t>(y));
}

/// @relates operation
constexpr operation operator^(uint32_t x, operation y) {
  return static_cast<operation>(x ^ static_cast<uint32_t>(y));
}

/// @relates operation
constexpr operation operator|(operation x, operation y) {
  return static_cast<operation>(static_cast<uint32_t>(x)
                                | static_cast<uint32_t>(y));
}

/// @relates operation
constexpr operation operator&(operation x, operation y) {
  return static_cast<operation>(static_cast<uint32_t>(x)
                                & static_cast<uint32_t>(y));
}

/// @relates operation
constexpr operation operator^(operation x, operation y) {
  return static_cast<operation>(static_cast<uint32_t>(x)
                                ^ static_cast<uint32_t>(y));
}

/// @relates operation
constexpr operation operator~(operation x) {
  return static_cast<operation>(~static_cast<uint32_t>(x));
}

std::string to_string(operation op);

} // namespace net
