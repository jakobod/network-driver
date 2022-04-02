/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include "net/ip/v4_address.hpp"

#include <cstddef>

namespace net::ip {

/// IPv4 Endpoint representation (v4_address + port).
class v4_endpoint {
public:
  constexpr v4_endpoint(v4_address address, std::uint16_t port)
    : address_{std::move(address)}, port_{port} {
    // nop
  }

  constexpr const v4_address& address() const noexcept {
    return address_;
  }

  constexpr std::uint16_t port() const noexcept {
    return port_;
  }

private:
  const v4_address address_;
  const std::uint16_t port_;
};

bool operator==(const v4_endpoint& lhs, const v4_endpoint& rhs);

bool operator!=(const v4_endpoint& lhs, const v4_endpoint& rhs);

std::string to_string(const v4_endpoint& addr);

error_or<v4_endpoint> parse_v4_endpoint(std::string str);

} // namespace net::ip
