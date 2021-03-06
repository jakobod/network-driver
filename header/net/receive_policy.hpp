/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <cstdint>

namespace net {

struct receive_policy {
  uint32_t min_size;
  uint32_t max_size;

  friend bool operator==(const receive_policy& lhs, const receive_policy& rhs) {
    return (lhs.min_size == rhs.min_size) && (lhs.max_size == rhs.max_size);
  }

  static constexpr receive_policy between(uint32_t min_size,
                                          uint32_t max_size) {
    return {min_size, max_size};
  }

  static constexpr receive_policy exactly(uint32_t size) {
    return {size, size};
  }

  static constexpr receive_policy up_to(uint32_t max_size) {
    return {1, max_size};
  }

  static constexpr receive_policy stop() {
    return {0, 0};
  }
};

} // namespace net
