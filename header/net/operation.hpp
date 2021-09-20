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
  read = EPOLLIN,
  write = EPOLLOUT,
  read_write = read | write,
  edge_triggered = EPOLLET,
  exclusive = EPOLLEXCLUSIVE,
  one_shot = EPOLLONESHOT,
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
