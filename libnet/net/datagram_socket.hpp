/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/fwd.hpp"

#include "net/socket.hpp"

#include "util/byte_span.hpp"
#include "util/error_or.hpp"

#include <cstddef>

namespace net {

/// A datagram-oriented network communication endpoint for packets.
struct datagram_socket : socket {
  using super = socket;

  using super::super;
};

/// Creates a datagram_socket
util::error_or<datagram_socket_pair> make_connected_datagram_socket_pair();

/// Receives packet from `x`.
ptrdiff_t read(datagram_socket x, util::byte_span buf);

/// Sends packet to `x`.
ptrdiff_t write(datagram_socket x, util::const_byte_span buf);

} // namespace net
