/**
 *  @author    Jakob Otto
 *  @file      pipe_socket.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "net/socket/socket.hpp"

#include <cstddef>
#include <utility>

struct iovec;

namespace net {

/// @brief Unidirectional communication endpoint for inter-process
/// communication.
struct pipe_socket : socket {
  using super = socket;

  using super::super;
};

/// @brief A pair of connected pipe sockets.
/// The first socket is for reading, the second for writing.
using pipe_socket_pair = std::pair<pipe_socket, pipe_socket>;

/// @brief Creates a pair of connected unidirectional pipe sockets.
/// The returned pair contains a read socket (first) and a write socket
/// (second). They are connected such that data written to the second can be
/// read from the first.
/// @return Either a pipe_socket_pair with connected sockets or an error.
[[nodiscard]] util::error_or<pipe_socket_pair> make_pipe();

/// @brief Reads data from a pipe socket (receiving side).
/// Retrieves data sent to this pipe from its peer.
/// @param x The pipe socket to read from (must be the read end of a pipe).
/// @param buf The buffer to receive data into.
/// @return The number of bytes read, or a negative value on error.
[[nodiscard]] std::ptrdiff_t read(pipe_socket x, util::byte_span buf);

/// @brief Writes data to a pipe socket (sending side).
/// Transmits the contents of the buffer to the connected peer.
/// @param x The pipe socket to write to (must be the write end of a pipe).
/// @param buf The data to transmit.
/// @return The number of bytes written, or a negative value on error.
[[nodiscard]] std::ptrdiff_t write(pipe_socket x, util::const_byte_span buf);

/// @brief Reads data from a pipe socket (receiving side).
/// Retrieves data sent to this pipe from its peer.
/// @param x The pipe socket to read from (must be the read end of a pipe).
/// @param buf The buffer to receive data into.
/// @return The number of bytes read, or a negative value on error.
[[nodiscard]] std::ptrdiff_t readv(pipe_socket x, std::span<iovec> iovecs);

/// @brief Writes data to a pipe socket (sending side).
/// Transmits the contents of the buffer to the connected peer.
/// @param x The pipe socket to write to (must be the write end of a pipe).
/// @param buf The data to transmit.
/// @return The number of bytes written, or a negative value on error.
[[nodiscard]] std::ptrdiff_t writev(pipe_socket x, std::span<iovec> iovecs);

} // namespace net
