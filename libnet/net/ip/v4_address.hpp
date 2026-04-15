/**
 *  @author    Jakob Otto
 *  @file      v4_address.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "util/byte_array.hpp"
#include "util/fwd.hpp"

#include <cstddef>
#include <functional>
#include <string>

namespace net::ip {

/// @brief Represents an IPv4 address with octet-based storage.
/// Provides both byte-oriented and bitwise access to the IPv4 address in
/// network-byte-order. Includes predefined constants for common addresses
/// (any, localhost, broadcast).
class v4_address {
  /// Number of octets required to represent an ipv4 address.
  static constexpr const std::size_t num_octets = 4;

public:
  /// Array type for representing a ipv4 address as bytes.
  using octet_array = util::byte_array<num_octets>;

  /// Predefined any address (0.0.0.0)
  static constexpr const octet_array any = util::make_byte_array(0, 0, 0, 0);
  /// Predefined localhost address (127.0.0.1)
  static constexpr const octet_array localhost = util::make_byte_array(127, 0,
                                                                       0, 1);
  /// Predefined local broadcast address (255.255.255.255)
  static constexpr const octet_array local_broadcast
    = util::make_byte_array(255, 255, 255, 255);

  /// @brief Default constructs an IPv4 address initialized to 0.0.0.0 (any
  /// address).
  constexpr v4_address() : bytes_{any} {
    // nop
  }

  /// @brief Constructs an IPv4 address from individual octets.
  /// @param bytes A byte array containing the four octets in
  /// network-byte-order.
  constexpr v4_address(octet_array bytes) : bytes_{bytes} {
    // nop
  }

  /// @brief Constructs an IPv4 address from a 32-bit integer.
  /// @param bits The address represented as a 32-bit value in
  /// network-byte-order.
  constexpr v4_address(std::uint32_t bits) : bits_(bits) {
    // nop
  }

  /// @brief Copy constructor.
  constexpr v4_address(const v4_address& ep) = default;

  /// @brief Move constructor.
  constexpr v4_address(v4_address&& ep) noexcept = default;

  /// @brief Copy assignment
  constexpr v4_address& operator=(const v4_address&) = default;

  /// @brief Move assignment
  constexpr v4_address& operator=(v4_address&&) noexcept = default;

  // -- Members ----------------------------------------------------------------

  /// @brief Returns the address as a 32-bit integer in network-byte-order.
  /// @return The bitwise representation of the address.
  constexpr std::uint32_t bits() const noexcept { return bits_; }

  /// @brief Returns the address as individual octets in network-byte-order.
  /// @return Reference to the byte array containing the four octets.
  constexpr const octet_array& bytes() const noexcept { return bytes_; }

private:
  /// Union for either representing the address in bitwise or bytewise
  /// representation.
  union {
    uint32_t bits_;     ///<< 32-bit representation
    octet_array bytes_; ///<< Octet array representation
  };
};

/// @relates v4_address
/// @brief Compares two IPv4 addresses for equality.
/// @param lhs The left operand.
/// @param rhs The right operand.
/// @return True if both addresses are identical.
bool operator==(const v4_address& lhs, const v4_address& rhs);

/// @relates v4_address
/// @brief Compares two IPv4 addresses for inequality.
/// @param lhs The left operand.
/// @param rhs The right operand.
/// @return True if the addresses differ.
bool operator!=(const v4_address& lhs, const v4_address& rhs);

/// @relates v4_address
/// @brief Converts an IPv4 address to its string representation.
/// Produces a dotted-decimal notation (e.g., "192.168.1.1").
/// @param addr The address to convert.
/// @return The string representation of the address.
std::string to_string(const v4_address& addr);

/// @relates v4_address
/// @brief Parses an IPv4 address from a string in dotted-decimal notation.
/// @param str The string to parse (e.g., "192.168.1.1").
/// @return The parsed address on success, or an error on failure.
util::error_or<v4_address> parse_v4_address(std::string_view str) noexcept;

} // namespace net::ip

namespace std {

template <>
struct hash<net::ip::v4_address> {
  std::size_t operator()(const net::ip::v4_address& addr) const noexcept {
    return std::hash<std::uint32_t>{}(addr.bits());
  }
};

} // namespace std
