/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/tcp_stream_socket.hpp"

#include "net/ip/v4_endpoint.hpp"
#include "net/socket_guard.hpp"

#include "util/error.hpp"

#include <netinet/tcp.h>

namespace net {

util::error_or<tcp_stream_socket>
make_connected_tcp_stream_socket(const ip::v4_endpoint& ep) {
  if (ep.port() == 0)
    return util::error(util::error_code::invalid_argument,
                       "port may not be zero");
  const tcp_stream_socket sock{::socket(AF_INET, SOCK_STREAM, 0)};
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

bool nodelay(tcp_stream_socket x, bool new_value) {
  int flag = new_value ? 1 : 0;
  auto res = setsockopt(x.id, IPPROTO_TCP, TCP_NODELAY,
                        reinterpret_cast<const void*>(&flag),
                        static_cast<int>(sizeof(flag)));
  return res == 0;
}

} // namespace net
