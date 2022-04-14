/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/tcp_stream_socket.hpp"

#include "net/ip/v4_endpoint.hpp"

#include "net/socket_guard.hpp"
#include "net/socket_sys_includes.hpp"

#include <iostream>

namespace net {

util::error_or<tcp_stream_socket>
make_connected_tcp_stream_socket(ip::v4_endpoint ep) {
  if (ep.port() == 0)
    return util::error(util::error_code::invalid_argument,
                       "port should not be zero");
  tcp_stream_socket sock{::socket(AF_INET, SOCK_STREAM, 0)};
  auto guard = make_socket_guard(sock);
  if (sock == invalid_socket)
    return util::error(util::error_code::socket_operation_failed,
                       last_socket_error_as_string());
  auto servaddr = to_sockaddr_in(ep);
  if (::connect(sock.id, reinterpret_cast<sockaddr*>(&servaddr),
                sizeof(sockaddr_in))
      != 0)
    return util::error(util::error_code::socket_operation_failed,
                       last_socket_error_as_string());
  return guard.release();
}

} // namespace net
