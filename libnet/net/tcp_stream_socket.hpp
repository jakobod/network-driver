/**
 *  @author    Jakob Otto
 *  @file      tcp_stream_socket.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "net/stream_socket.hpp"

namespace net {

/// Represents a TCP connection.
struct tcp_stream_socket : stream_socket {
  using super = stream_socket;

  using super::super;
};

/// Create a `tcp_stream_socket` connected to `ep`.
util::error_or<tcp_stream_socket>
make_connected_tcp_stream_socket(const ip::v4_endpoint& ep);

/// Enables or disables Nagle's algorithm on `x`.
bool nodelay(tcp_stream_socket x, bool new_value);

} // namespace net
