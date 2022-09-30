/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "util/byte_array.hpp"
#include "util/error_or.hpp"

#include <cstddef>
#include <string_view>

namespace net::ip {

/// IPv4 Address representation.
class v4_address {
  /// Number of octets required to represent an ipv4 address.
  static constexpr const std::size_t num_octets = 4;

public:
  /// Array type for representing a ipv4 address as bytes.
  using octet_array = util::byte_array<num_octets>;

  /// Predefined any address
  static constexpr const octet_array any = util::make_byte_array(0, 0, 0, 0);
  /// Predefined localhost address
  static constexpr const octet_array localhost = util::make_byte_array(127, 0,
                                                                       0, 1);
  /// Predefined localhost address
  static constexpr const octet_array local_broadcast
    = util::make_byte_array(255, 255, 255, 255);

  /// Default initialization of an address
  constexpr v4_address() : bytes_{any} {
    // nop
  }

  /// Constructs an ipv4 address from bytes.
  constexpr v4_address(octet_array bytes) : bytes_{bytes} {
    // nop
  }

  /// Constructs an ipv4 address from bits in network-byte-order.
  constexpr v4_address(std::uint32_t bits) : bits_(bits) {
    // nop
  }

  /// Move constructor
  v4_address(v4_address&& ep) noexcept = default;

  /// Default copy constructor
  v4_address(const v4_address& ep) = default;

  // -- Members ----------------------------------------------------------------

  /// returns the address in bitwise representation in network-byte-order.
  constexpr std::uint32_t bits() const noexcept {
    return bits_;
  }

  /// returns the address in bytewise representation in network-byte-order.
  constexpr const octet_array& bytes() const noexcept {
    return bytes_;
  }

private:
  /// Union for either representing the address in bitwise or bytewise
  /// representation.
  union {
    const uint32_t bits_;
    const octet_array bytes_;
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
