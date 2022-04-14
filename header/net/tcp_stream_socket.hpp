/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include "net/stream_socket.hpp"

#include "util/error.hpp"

namespace net {

/// Represents a TCP connection.
struct tcp_stream_socket : stream_socket {
  using super = stream_socket;

  using super::super;
};

/// Create a `tcp_stream_socket` connected to `ep`.
util::error_or<tcp_stream_socket>
make_connected_tcp_stream_socket(ip::v4_endpoint ep);

} // namespace net
