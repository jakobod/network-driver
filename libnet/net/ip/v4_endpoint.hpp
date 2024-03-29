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

/// IPv4 Endpoint representation (v4_address + port).
class v4_endpoint {
public:
  v4_endpoint(sockaddr_in saddr);

  constexpr v4_endpoint(v4_address address, std::uint16_t port)
    : address_{address}, port_{port} {
    // nop
  }

  constexpr v4_endpoint() : address_{v4_address::any} {
    // nop
  }

  /// Move constructor
  v4_endpoint(v4_endpoint&& ep) noexcept = default;

  /// Default copy constructor
  v4_endpoint(const v4_endpoint& ep) = default;

  // -- Members ----------------------------------------------------------------

  /// Returns the contained v4 address
  constexpr const v4_address& address() const noexcept { return address_; }

  /// Returns the contained port
  constexpr std::uint16_t port() const noexcept { return port_; }

private:
  const v4_address address_;
  const std::uint16_t port_{0};
};

bool operator==(const v4_endpoint& lhs, const v4_endpoint& rhs);

bool operator!=(const v4_endpoint& lhs, const v4_endpoint& rhs);

std::string to_string(const v4_endpoint& addr);

util::error_or<v4_endpoint> parse_v4_endpoint(const std::string& str);

sockaddr_in to_sockaddr_in(const v4_endpoint& ep);

} // namespace net::ip
