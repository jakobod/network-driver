/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 31.03.2021
 */

#pragma once

#include <cstdint>

#include "detail/error.hpp"
#include "net/fwd.hpp"
#include "net/socket.hpp"

namespace net {

constexpr int max_conn_backlog = 10;

/// Represents a TCP connection.
struct tcp_accept_socket : socket {
  using super = socket;

  using super::super;
};

using acceptor_pair = std::pair<tcp_accept_socket, uint16_t>;

detail::error_or<acceptor_pair> make_tcp_accept_socket(uint16_t port);

detail::error_or<uint16_t> port_of(tcp_accept_socket x);

bool reuseaddr(socket x, bool new_value);

tcp_stream_socket accept(tcp_accept_socket sock);

} // namespace net
