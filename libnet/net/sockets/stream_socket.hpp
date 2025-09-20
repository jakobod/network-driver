/**
 *  @author    Jakob Otto
 *  @file      stream_socket.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/sockets/socket.hpp"

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include <cstddef>
#include <utility>

namespace net::sockets {

/// @brief Stream oriented socket.
struct stream_socket : socket {
  /// @brief Alias for the base type
  using super = socket;
  /// @brief inherited constructor from base class
  using super::super;
};

/// @brief Alias for a pair of stream sockets
using stream_socket_pair = std::pair<stream_socket, stream_socket>;

/// @brief Creates a connected stream_socket_pair
/// @details The sockets are unix domain sockets
/// @returns a pair of connected sockets or an error in case of failure
util::error_or<stream_socket_pair> make_stream_socket_pair();

/// @brief Enables or disables keepalive on `hdl`.
/// @param hdl  The socket to enable keepalive for
/// @param enable  Whether to enable keepalive
/// @returns true on success, false otherwise
bool keepalive(stream_socket hdl, bool enable);

/// @brief Receives data from `hdl`.
/// @param hdl  The socket to read from
/// @param buf  The buffer to read to, size controls the maximum number of bytes
/// @returns the number of bytes read, -1 in case of error
ptrdiff_t read(stream_socket hdl, util::byte_span buf);

/// @brief Sends data to `hdl`.
/// @param hdl  The socket to write to
/// @param buf  The buffer to write the data from
/// @returns the number of bytes read, -1 in case of error
ptrdiff_t write(stream_socket hdl, util::const_byte_span buf);

} // namespace net::sockets
