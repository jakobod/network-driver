/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *
 *  This file is based on `tcp_stream_socket.hpp` from the C++ Actor Framework.
 *  https://github.com/actor-framework/incubator
 */

#pragma once

#include "net/stream_socket.hpp"

#include <cstdint>
#include <string>

#include "net/error.hpp"

namespace net {

/// Represents a TCP connection.
struct tcp_stream_socket : stream_socket {
  using super = stream_socket;

  using super::super;
};

/// Create a `tcp_stream_socket` connected to `auth`.
error_or<tcp_stream_socket>
make_connected_tcp_stream_socket(const std::string hostname, uint16_t port);

} // namespace net
