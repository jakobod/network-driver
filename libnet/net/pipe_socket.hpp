/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "util/error.hpp"

#include "net/socket.hpp"

#include <utility>

namespace net {

/// A unidirectional communication endpoint for inter-process communication.
struct pipe_socket : socket {
  using super = socket;

  using super::super;
};

/// A pair of pipe sockets
using pipe_socket_pair = std::pair<pipe_socket, pipe_socket>;

/// Creates two connected sockets. The first socket is the read handle and the
/// second socket is the write handle.
util::error_or<pipe_socket_pair> make_pipe();

/// Transmits data from `x` to its peer.
[[nodiscard]] std::ptrdiff_t write(pipe_socket x, util::const_byte_span buf);

/// Receives data from `x`.
[[nodiscard]] std::ptrdiff_t read(pipe_socket x, util::byte_span buf);

} // namespace net
