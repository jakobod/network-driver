/**
 *  @author    Jakob Otto
 *  @file      datagram_socket.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "net/socket.hpp"

#include "util/fwd.hpp"

#include <cstddef>
#include <utility>

namespace net {

/// A datagram-oriented network communication endpoint for packets.
struct datagram_socket : socket {
  using super = socket;

  using super::super;
};

/// Pair of datagram sockets
using datagram_socket_pair = std::pair<datagram_socket, datagram_socket>;

/// Creates a datagram_socket
util::error_or<datagram_socket_pair> make_connected_datagram_socket_pair();

/// Receives packet from `x`.
std::ptrdiff_t read(datagram_socket x, util::byte_span buf);

/// Sends packet to `x`.
std::ptrdiff_t write(datagram_socket x, util::const_byte_span buf);

} // namespace net
