/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/fwd.hpp"

#include "net/datagram_socket.hpp"

namespace net {

/// Represents a TCP connection.
struct udp_datagram_socket : datagram_socket {
  using super = datagram_socket;

  using super::super;
};

using udp_datagram_socket_result
  = std::pair<udp_datagram_socket, std::uint16_t>;
using udp_read_result = std::pair<ip::v4_endpoint, std::ptrdiff_t>;

/// Create a `udp_datagram_socket` bound to `port`.
util::error_or<udp_datagram_socket_result>
make_udp_datagram_socket(std::uint16_t port);

/// Sends packet to `x`.
std::ptrdiff_t write(udp_datagram_socket x, ip::v4_endpoint ep,
                     util::const_byte_span buf);

/// Receives packet from `x`.
udp_read_result read(udp_datagram_socket x, util::byte_span buf);

} // namespace net
