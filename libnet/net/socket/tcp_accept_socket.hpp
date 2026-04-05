/**
 *  @author    Jakob Otto
 *  @file      tcp_accept_socket.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "net/socket/socket.hpp"

#include <cstdint>

namespace net {

/// @brief TCP socket for accepting incoming connections.
struct tcp_accept_socket : socket {
  using super = socket;

  using super::super;
};

/// @brief Pair containing a listening TCP socket and its bound port.
/// Returned when creating a listening socket to provide both the socket
/// and the actual port it was bound to.
using acceptor_pair = std::pair<tcp_accept_socket, uint16_t>;

/// @brief Configures a TCP socket to listen for incoming connections.
/// Must be called before accepting connections on the socket.
/// @param sock The TCP accept socket to configure.
/// @param conn_backlog The maximum number of pending connections allowed.
/// @return An error if the operation fails, or success otherwise.
util::error listen(tcp_accept_socket sock, int conn_backlog);

/// @brief Accepts an incoming TCP connection from a listening socket.
/// Blocks until a client connection is available. The returned socket
/// represents the new client connection.
/// @param sock The listening TCP accept socket.
/// @return A new tcp_stream_socket representing the accepted client connection.
tcp_stream_socket accept(tcp_accept_socket sock);

/// @brief Creates and listens on a TCP socket bound to the specified endpoint.
/// The socket is automatically set to listen mode with the specified backlog.
/// @param ep The IPv4 endpoint (address and port) to bind and listen on.
/// @param conn_backlog The maximum number of pending connections (default: 10).
/// @return Either an acceptor_pair (socket and bound port) or an error.
util::error_or<acceptor_pair>
make_tcp_accept_socket(const ip::v4_endpoint& ep, const int conn_backlog = 10);

} // namespace net
