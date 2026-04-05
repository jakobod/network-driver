/**
 *  @author    Jakob Otto
 *  @file      datagram_socket.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "net/socket/socket.hpp"

#include "util/fwd.hpp"

#include <cstddef>
#include <utility>

namespace net {

/// @brief Datagram-oriented socket for UDP communication.
struct datagram_socket : socket {
  using super = socket;

  using super::super;
};

/// @brief A pair of connected datagram sockets.
using datagram_socket_pair = std::pair<datagram_socket, datagram_socket>;

/// @brief Creates a connected pair of datagram sockets.
/// Creates two UDP sockets that are connected to each other for bidirectional
/// datagram communication.
/// @return A pair of connected datagram sockets or an error if creation failed.
[[nodiscard]] util::error_or<datagram_socket_pair>
make_connected_datagram_socket_pair();

/// @brief Receives a packet from a datagram socket. Reads a single datagram
/// (packet) from the socket.
/// @param x The datagram socket to read from.
/// @param buf Buffer to store the received packet data.
/// @return The number of bytes read, or -1 on error.
[[nodiscard]] std::ptrdiff_t read(datagram_socket x, util::byte_span buf);

/// @brief Sends a packet to a datagram socket. Writes a single datagram
/// (packet) to the socket.
/// @param x The datagram socket to write to.
/// @param buf The packet data to send.
/// @return The number of bytes written, or -1 on error.
[[nodiscard]] std::ptrdiff_t write(datagram_socket x,
                                   util::const_byte_span buf);

} // namespace net
