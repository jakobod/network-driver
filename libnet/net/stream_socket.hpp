/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/socket.hpp"

#include "net/fwd.hpp"

namespace net {

/// Stream oriented socket.
struct stream_socket : socket {
  using super = socket;

  using super::super;
};

/// A pair of stream sockets
using stream_socket_pair = std::pair<stream_socket, stream_socket>;

/// Creates a connected stream_socket_pair (unix domain sockets)
util::error_or<stream_socket_pair> make_stream_socket_pair();

/// Enables or disables keepalive on `x`.
bool keepalive(stream_socket x, bool new_value);

/// Receives data from `x`.
ptrdiff_t read(stream_socket x, util::byte_span buf);

/// Sends data to `x`.
ptrdiff_t write(stream_socket x, util::const_byte_span buf);

} // namespace net
