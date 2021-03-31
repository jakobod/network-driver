/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 31.03.2021
 */

#pragma once

#include <cstdint>

#include "net/fwd.hpp"
#include "net/socket.hpp"

namespace net {

constexpr int max_conn_backlog = 10;

/// Represents a TCP connection.
struct tcp_accept_socket : socket {
  using super = socket;

  using super::super;
};

tcp_accept_socket make_tcp_accept_socket(uint16_t port);

tcp_stream_socket accept(tcp_accept_socket sock);

} // namespace net
