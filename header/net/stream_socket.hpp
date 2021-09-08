/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 31.03.2021
 *
 *  This file is based on `stream_socket.hpp` from the C++ Actor Framework.
 *  https://github.com/actor-framework/incubator
 */

#pragma once

#include <cstddef>

#include "detail/error.hpp"
#include "fwd.hpp"
#include "net/socket.hpp"

namespace net {

/// A connection-oriented network communication endpoint for bidirectional byte
/// streams.
struct stream_socket : socket {
  using super = socket;

  using super::super;
};

/// Creates a connected stream_socket_pair
detail::error_or<stream_socket_pair> make_stream_socket_pair();

/// Enables or disables keepalive on `x`.
bool keepalive(stream_socket x, bool new_value);

/// Enables or disables Nagle's algorithm on `x`.
bool nodelay(stream_socket x, bool new_value);

/// Enables or disables reuseaddr on `x`.
bool reuseaddr(socket x, bool new_value);

/// Receives data from `x`.
ptrdiff_t read(stream_socket x, detail::byte_span buf);

/// Sends data to `x`.
ptrdiff_t write(stream_socket x, detail::byte_span buf);

} // namespace net
