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
  /// Number of octets required to represent an ipv4 address.
  static constexpr const std::size_t num_octets = 4;

public:
  /// Array type for representing a ipv4 address as bytes.
  using byte_array = util::byte_array<num_octets>;

  /// Constructs an ipv4 address from bytes.
  constexpr v4_address(byte_array bytes) : bytes_{std::move(bytes)} {
    // nop
  }

  /// Constructs an ipv4 address from bits in network-byte-order.
  constexpr v4_address(std::uint32_t bits) : bits_(bits) {
    // nop
  }

  /// returns the address in bitwise representation in network-byte-order.
  constexpr std::uint32_t bits() const noexcept {
    return bits_;
  }

  /// returns the address in bytewise representation in network-byte-order.
  constexpr const byte_array& bytes() const noexcept {
    return bytes_;
  }

private:
  /// Union for either representing the address in bitwise or bytewise
  /// representation.
  union {
    const uint32_t bits_;
    const byte_array bytes_;
  };
};

/// Compares two v4_addresses for equality.
bool operator==(const v4_address& lhs, const v4_address& rhs);

/// Compares two v4_addresses for inequality.
bool operator!=(const v4_address& lhs, const v4_address& rhs);

/// Returns a v4_address as string.
std::string to_string(const v4_address& addr);

/// parses a v4_address from string.
util::error_or<v4_address> parse_v4_address(const std::string& str);

} // namespace net::ip
