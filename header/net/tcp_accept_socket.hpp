/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <cstdint>

#include "fwd.hpp"
#include "net/error.hpp"
#include "net/socket.hpp"

namespace net {

constexpr int max_conn_backlog = 10;

/// Represents a TCP connection.
struct tcp_accept_socket : socket {
  using super = socket;

  using super::super;
};

using acceptor_pair = std::pair<tcp_accept_socket, uint16_t>;

error_or<acceptor_pair> make_tcp_accept_socket(uint16_t port);

error_or<uint16_t> port_of(tcp_accept_socket x);

bool reuseaddr(socket x, bool new_value);

tcp_stream_socket accept(tcp_accept_socket sock);

} // namespace net
