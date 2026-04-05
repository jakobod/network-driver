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

/// @brief Socket operation types for multiplexed I/O.
/// Defines the operations that can be monitored on sockets, matching
/// the epoll event masks for compatibility.
enum class operation : std::uint8_t {
  /// No operation (idle state).
  none = 0x00,
  /// Monitor for read/receive readiness.
  read = 0x01,
  /// Monitor for write/send readiness.
  write = 0x02,
  /// Monitor for both read and write readiness.
  read_write = read | write,
  /// Monitor for incoming connections (accept).
  accept = 0x04,
  /// Monitor for read and accept readiness.
  read_accept = read | accept,
};

/// @brief Bitwise OR operator for combining operations with uint8_t.
/// @relates operation
/// @param x The left operand (uint8_t).
/// @param y The right operand (operation).
/// @return The combined operation.
constexpr operation operator|(std::uint8_t x, operation y) {
  return static_cast<operation>(x | static_cast<std::uint8_t>(y));
}

/// @brief Bitwise AND operator for operations with uint8_t.
/// @relates operation
/// @param x The left operand (uint8_t).
/// @param y The right operand (operation).
/// @return The masked operation.
constexpr operation operator&(std::uint8_t x, operation y) {
  return static_cast<operation>(x & static_cast<std::uint8_t>(y));
}

/// @brief Bitwise XOR operator for operations with uint8_t.
/// @relates operation
/// @param x The left operand (uint8_t).
/// @param y The right operand (operation).
/// @return The XOR result as operation.
constexpr operation operator^(std::uint8_t x, operation y) {
  return static_cast<operation>(x ^ static_cast<std::uint8_t>(y));
}

/// @brief Bitwise OR operator for combining two operations.
/// @relates operation
/// @param x The first operation.
/// @param y The second operation.
/// @return The combined operation.
constexpr operation operator|(operation x, operation y) {
  return static_cast<operation>(static_cast<std::uint8_t>(x)
                                | static_cast<std::uint8_t>(y));
}

/// @brief Bitwise AND operator for masking operations.
/// @relates operation
/// @param x The first operation.
/// @param y The second operation.
/// @return The masked operation.
constexpr operation operator&(operation x, operation y) {
  return static_cast<operation>(static_cast<std::uint8_t>(x)
                                & static_cast<std::uint8_t>(y));
}

/// @brief Bitwise XOR operator for two operations.
/// @relates operation
/// @param x The first operation.
/// @param y The second operation.
/// @return The XOR result as operation.
constexpr operation operator^(operation x, operation y) {
  return static_cast<operation>(static_cast<std::uint8_t>(x)
                                ^ static_cast<std::uint8_t>(y));
}

/// @brief Bitwise NOT operator for inverting operation bits.
/// @relates operation
/// @param x The operation to invert.
/// @return The bitwise NOT result as operation.
constexpr operation operator~(operation x) {
  return static_cast<operation>(~static_cast<std::uint8_t>(x));
}

/// @brief Converts an operation enum to its string representation.
/// @param op The operation to convert.
/// @return A string describing the operation (e.g., "read", "write",
/// "read_write").
std::string to_string(operation op);

/// @brief Outputs an operation to a stream.
/// @param os The output stream.
/// @param op The operation to output.
/// @return The output stream for chaining.
std::ostream& operator<<(std::ostream& os, operation op);

} // namespace net
