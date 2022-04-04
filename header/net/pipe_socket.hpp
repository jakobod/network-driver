/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *
 *  This file is based on `pipe_socket.cpp` from the C++ Actor Framework.
 *  https://github.com/actor-framework/incubator
 */

#pragma once

#include "fwd.hpp"

#include "net/socket.hpp"

#include "util/error.hpp"

#include <cstddef>
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
ptrdiff_t write(pipe_socket x, util::byte_span buf);

/// Receives data from `x`.
ptrdiff_t read(pipe_socket x, util::byte_span buf);

} // namespace net
