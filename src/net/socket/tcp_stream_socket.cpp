/**
 *  @author    Jakob Otto
 *  @file      tcp_stream_socket.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/socket/tcp_stream_socket.hpp"

#include "net/ip/v4_endpoint.hpp"
#include "net/socket_guard.hpp"

#include "util/error.hpp"
#include "util/logger.hpp"

#include <netinet/tcp.h>

namespace net {

util::error_or<tcp_stream_socket>
make_connected_tcp_stream_socket(const ip::v4_endpoint& ep) {
  if (ep.port() == 0)
    return util::error(util::error_code::invalid_argument,
                       "port may not be zero");
  const tcp_stream_socket sock{::socket(AF_INET, SOCK_STREAM, 0)};
  LOG_DEBUG("Created new socket with ", NET_ARG2("id", sock.id));
  auto guard = make_socket_guard(sock);
  if (sock == invalid_socket)
    return util::error(util::error_code::socket_operation_failed,
                       last_socket_error_as_string());
  auto servaddr = to_sockaddr_in(ep);
  LOG_DEBUG("Connecting with ", NET_ARG2("socket", sock.id), " to ",
            NET_ARG2("endpoint", to_string(ep)));
  if (::connect(sock.id, reinterpret_cast<sockaddr*>(&servaddr),
                sizeof(sockaddr_in))
      != 0)
    return util::error(util::error_code::socket_operation_failed,
                       last_socket_error_as_string());
  return guard.release();
}

bool nodelay(tcp_stream_socket hdl, bool new_value) {
  LOG_DEBUG("keepalive on ", NET_ARG2("tcp_stream_socket", hdl.id), ", ",
            NET_ARG(new_value));
  int flag = new_value ? 1 : 0;
  return ((setsockopt(hdl.id, IPPROTO_TCP, TCP_NODELAY,
                      reinterpret_cast<const void*>(&flag),
                      static_cast<int>(sizeof(flag))))
          == 0);
}

} // namespace net
