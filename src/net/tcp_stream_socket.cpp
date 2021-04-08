/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 31.03.2021
 *
 *  This file is based on `tcp_stream_socket.cpp` from the C++ Actor Framework.
 *  https://github.com/actor-framework/incubator
 */

#include "net/tcp_stream_socket.hpp"

#include <iostream>

#include "detail/socket_guard.hpp"
#include "net/socket_sys_includes.hpp"

namespace net {

detail::error_or<tcp_stream_socket>
make_connected_tcp_stream_socket(const std::string ip, uint16_t port) {
  if (port == 0)
    return detail::error(detail::invalid_argument, "port should not be zero");
  tcp_stream_socket sock{::socket(AF_INET, SOCK_STREAM, 0)};
  auto guard = detail::make_socket_guard(sock);
  if (sock == invalid_socket)
    return detail::error(detail::socket_operation_failed,
                         last_socket_error_as_string());
  sockaddr_in servaddr = {};
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(ip.c_str());
  servaddr.sin_port = htons(port);
  if (::connect(sock.id, (sockaddr*) &servaddr, sizeof(sockaddr_in)) != 0)
    return detail::error(detail::socket_operation_failed,
                         last_socket_error_as_string());
  return guard.release();
}

} // namespace net
