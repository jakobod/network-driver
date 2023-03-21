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

#include "net/socket.hpp"

#include <cstdint>

namespace net {

/// Represents a TCP connection.
struct tcp_accept_socket : socket {
  using super = socket;

  using super::super;
};

/// A pair containing a tcp_accept_socket and its bound port
using acceptor_pair = std::pair<tcp_accept_socket, uint16_t>;

/// Sets `sock` to listen for incoming connections
util::error listen(tcp_accept_socket sock, int conn_backlog);

/// Accepts an incoming connection from sock
tcp_stream_socket accept(tcp_accept_socket sock);

/// Creates a tcp_accept_socket that is bound to `port`
util::error_or<acceptor_pair>
make_tcp_accept_socket(const ip::v4_endpoint& ep, const int conn_backlog = 10);

} // namespace net
