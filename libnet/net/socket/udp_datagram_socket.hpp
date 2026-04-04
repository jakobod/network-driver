/**
 *  @author    Jakob Otto
 *  @file      udp_datagram_socket.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "net/socket/datagram_socket.hpp"

namespace net {

/// @brief UDP datagram socket for connectionless communication.
/// Extends datagram_socket with UDP-specific functionality for sending
/// and receiving independent datagrams.
struct udp_datagram_socket : datagram_socket {
  using super = datagram_socket;

  using super::super;
};

/// @brief Pair containing a UDP socket and its bound port.
/// Returned when creating a UDP socket to provide both the socket
/// and the actual port it was bound to.
using udp_datagram_socket_result
  = std::pair<udp_datagram_socket, std::uint16_t>;

/// @brief Result type for UDP receive operations.
/// Contains the source endpoint of the received packet and the number of bytes
/// read.
using udp_read_result = std::pair<ip::v4_endpoint, std::ptrdiff_t>;

/// @brief Creates a UDP datagram socket bound to a specific port.
/// The socket is ready to send and receive datagrams immediately.
/// @param port The port number to bind to (0 for automatic assignment).
/// @return Either a udp_datagram_socket_result (socket and bound port) or an
/// error.
util::error_or<udp_datagram_socket_result>
make_udp_datagram_socket(std::uint16_t port);

/// @brief Sends a UDP datagram to a remote endpoint.
/// No connection is required; the datagram is sent directly to the target.
/// @param x The UDP datagram socket to send from.
/// @param ep The destination IPv4 endpoint (address and port).
/// @param buf The data to send.
/// @return The number of bytes sent, or a negative value on error.
std::ptrdiff_t write(udp_datagram_socket x, ip::v4_endpoint ep,
                     util::const_byte_span buf);

/// @brief Receives a UDP datagram from any source.
/// Returns both the source endpoint and the data received.
/// @param x The UDP datagram socket to receive from.
/// @param buf The buffer to receive data into.
/// @return A pair containing the source endpoint and number of bytes read.
udp_read_result read(udp_datagram_socket x, util::byte_span buf);

} // namespace net
