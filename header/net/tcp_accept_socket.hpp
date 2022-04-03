/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include "net/error.hpp"
#include "net/socket.hpp"

#include <cstdint>

namespace net {

/// Represents a TCP connection.
struct tcp_accept_socket : socket {
  using super = socket;

  using super::super;
};

using acceptor_pair = std::pair<tcp_accept_socket, uint16_t>;

error listen(tcp_accept_socket sock, int conn_backlog);

tcp_stream_socket accept(tcp_accept_socket sock);

error_or<acceptor_pair> make_tcp_accept_socket(uint16_t port);

} // namespace net
