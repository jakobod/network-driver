/**
 *  @author    Jakob Otto
 *  @file      stream_socket.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/socket/socket.hpp"

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include <cstddef>
#include <utility>

namespace net {

/// @brief Stream-oriented socket for TCP connections.
/// Inherits from `socket` and adds stream-specific operations like
/// sending and receiving data with keepalive support.
struct stream_socket : socket {
  using super = socket;

  using super::super;
};

/// @brief A pair of connected stream sockets.
using stream_socket_pair = std::pair<stream_socket, stream_socket>;

/// @brief Creates a connected pair of stream sockets (unix domain sockets).
/// Useful for inter-process communication or creating bidirectional pipes.
/// @return A pair of connected sockets or an error if creation failed.
util::error_or<stream_socket_pair> make_stream_socket_pair();

/// @brief Enables or disables TCP keepalive on a stream socket.
/// Sends periodic probes to detect broken connections.
/// @param x The stream socket to modify.
/// @param new_value true to enable keepalive, false to disable.
/// @return true if the operation succeeded, false otherwise.
bool keepalive(stream_socket x, bool new_value);

/// @brief Receives data from a stream socket.
/// Reads up to buf.size() bytes from the socket.
/// @param x The stream socket to read from.
/// @param buf Buffer to store received data.
/// @return The number of bytes read, or -1 on error.
ptrdiff_t read(stream_socket x, util::byte_span buf);

/// @brief Sends data to a stream socket.
/// Writes up to buf.size() bytes to the socket.
/// @param x The stream socket to write to.
/// @param buf The data to send.
/// @return The number of bytes written, or -1 on error.
ptrdiff_t write(stream_socket x, util::const_byte_span buf);

} // namespace net
