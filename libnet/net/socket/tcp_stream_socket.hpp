/**
 *  @author    Jakob Otto
 *  @file      tcp_stream_socket.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "net/socket/stream_socket.hpp"

namespace net {

/// @brief TCP stream socket for bidirectional communication.
/// Extends stream_socket with TCP-specific functionality for establishing
/// and managing TCP connections.
struct tcp_stream_socket : stream_socket {
  using super = stream_socket;

  using super::super;
};

/// @brief Creates and connects a TCP stream socket to a remote endpoint.
/// @param ep The IPv4 endpoint (address and port) to connect to.
/// @return Either a connected tcp_stream_socket or an error.
util::error_or<tcp_stream_socket>
make_connected_tcp_stream_socket(const ip::v4_endpoint& ep);

/// @brief Enables or disables Nagle's algorithm (TCP_NODELAY option).
/// When disabled (nodelay=true), data is sent immediately without waiting
/// for more data to accumulate. When enabled (nodelay=false), small segments
/// are buffered and sent together.
/// @param x The TCP stream socket to configure.
/// @param new_value True to disable Nagle's algorithm, false to enable it.
/// @return True if the operation succeeded, false otherwise.
bool nodelay(tcp_stream_socket x, bool new_value);

} // namespace net
