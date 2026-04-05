/**
 *  @author    Jakob Otto
 *  @file      v4_endpoint.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "net/ip/v4_address.hpp"

#include <cstddef>
#include <netinet/ip.h>
#include <string>

namespace net::ip {

/// @brief Represents an IPv4 endpoint (address + port pair).
/// An endpoint uniquely identifies a peer on the network by combining an IPv4
/// address with a transport layer port number. Supports conversion to/from
/// sockaddr_in for socket operations.
class v4_endpoint {
public:
  /// @brief Constructs an endpoint from a sockaddr_in structure.
  /// @param saddr The socket address structure (from socket API).
  v4_endpoint(sockaddr_in saddr);

  /// @brief Constructs an endpoint from an address and port.
  /// @param address The IPv4 address.
  /// @param port The port number.
  constexpr v4_endpoint(v4_address address, std::uint16_t port)
    : address_{address}, port_{port} {
    // nop
  }

  /// @brief Default constructs an endpoint initialized to 0.0.0.0:0.
  constexpr v4_endpoint() : address_{v4_address::any} {
    // nop
  }

  /// @brief Copy constructor.
  constexpr v4_endpoint(const v4_endpoint& ep) = default;

  /// @brief Move constructor.
  constexpr v4_endpoint(v4_endpoint&& ep) noexcept = default;

  /// @brief Copy assignment
  constexpr v4_endpoint& operator=(const v4_endpoint&) = default;

  /// @brief Move assignment
  constexpr v4_endpoint& operator=(v4_endpoint&&) noexcept = default;

  // -- Members ----------------------------------------------------------------

  /// @brief Returns the IPv4 address component of this endpoint.
  /// @return Const reference to the contained address.
  constexpr const v4_address& address() const noexcept { return address_; }

  /// @brief Returns the port number component of this endpoint.
  /// @return The port in network-byte-order.
  constexpr std::uint16_t port() const noexcept { return port_; }

private:
  v4_address address_;    ///< The IPv4 address
  std::uint16_t port_{0}; ///< The port number
};

/// @relates v4_endpoint
/// @brief Compares two IPv4 endpoints for equality.
/// @param lhs The left operand.
/// @param rhs The right operand.
/// @return True if both endpoints match (same address and port).
bool operator==(const v4_endpoint& lhs, const v4_endpoint& rhs);

/// @relates v4_endpoint
/// @brief Compares two IPv4 endpoints for inequality.
/// @param lhs The left operand.
/// @param rhs The right operand.
/// @return True if the endpoints differ.
bool operator!=(const v4_endpoint& lhs, const v4_endpoint& rhs);

/// @relates v4_endpoint
/// @brief Converts an IPv4 endpoint to its string representation.
/// Produces a colon-separated format (e.g., "192.168.1.1:8080").
/// @param addr The endpoint to convert.
/// @return The string representation of the endpoint.
std::string to_string(const v4_endpoint& addr);

/// @relates v4_endpoint
/// @brief Parses an IPv4 endpoint from a string.
/// Expects colon-separated format: address:port (e.g., "192.168.1.1:8080").
/// @param str The string to parse.
/// @return The parsed endpoint on success, or an error on failure.
util::error_or<v4_endpoint> parse_v4_endpoint(std::string_view str);

/// @relates v4_endpoint
/// @brief Converts an IPv4 endpoint to a socket address structure.
/// Produces a sockaddr_in suitable for socket API operations.
/// @param ep The endpoint to convert.
/// @return The socket address structure.
sockaddr_in to_sockaddr_in(const v4_endpoint& ep);

} // namespace net::ip
