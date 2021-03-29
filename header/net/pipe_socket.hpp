/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 04.03.2021
 *
 *  This file is based on `pipe_socket.cpp` from the C++ Actor Framework.
 *  https://github.com/actor-framework/incubator
 */

#pragma once

#include <utility>

#include "detail/byte_container.hpp"
#include "net/socket.hpp"

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
pipe_socket_pair make_pipe();

/// Transmits data from `x` to its peer.
ptrdiff_t write(pipe_socket x, detail::byte_span buf);

/// Receives data from `x`.
ptrdiff_t read(pipe_socket x, detail::byte_span buf);

} // namespace net
