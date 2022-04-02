/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include <cstddef>
#include <string_view>

namespace net::ip {

/// IPv4 Address representation.
class v4_address {
  static constexpr const std::size_t num_octets = 4;

public:
  using byte_array = util::byte_array<num_octets>;

  constexpr v4_address(byte_array bytes) : bytes_{std::move(bytes)} {
    // nop
  }

  constexpr v4_address(std::uint32_t bits) : bits_(bits) {
    // nop
  }

  constexpr std::uint32_t bits() const noexcept {
    return bits_;
  }

  constexpr const byte_array& bytes() const noexcept {
    return bytes_;
  }

private:
  union {
    const uint32_t bits_;
    const byte_array bytes_;
  };
};

bool operator==(const v4_address& lhs, const v4_address& rhs);

bool operator!=(const v4_address& lhs, const v4_address& rhs);

std::string to_string(const v4_address& addr);

error_or<v4_address> parse_v4_address(std::string str);

} // namespace net::ip
