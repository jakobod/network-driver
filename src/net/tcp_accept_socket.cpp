/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/tcp_accept_socket.hpp"

#include <iostream>
#include <utility>
#include <variant>

#include "net/error.hpp"
#include "net/socket_guard.hpp"
#include "net/socket_sys_includes.hpp"
#include "net/tcp_stream_socket.hpp"

namespace net {

error_or<acceptor_pair> make_tcp_accept_socket(uint16_t port) {
  sockaddr_in servaddr = {};
  tcp_accept_socket sock{::socket(AF_INET, SOCK_STREAM, 0)};
  auto guard = make_socket_guard(sock);
  if (sock == invalid_socket)
    return error(socket_operation_failed, "Failed to create socket");
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);
  if ((::bind(sock.id, reinterpret_cast<sockaddr*>(&servaddr),
              sizeof(sockaddr)))
      != 0)
    return error(socket_operation_failed,
                 "Failed to bind socket: " + last_socket_error_as_string());
  if ((listen(sock.id, max_conn_backlog)) != 0)
    return error(socket_operation_failed, "Failed to listen");
  auto res = net::port_of(*guard);
  if (auto err = get_error(res))
    return *err;
  return std::make_pair(guard.release(), std::get<uint16_t>(res));
}

error_or<uint16_t> port_of(tcp_accept_socket x) {
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  if (getsockname(x.id, (struct sockaddr*) &sin, &len) == -1)
    return error(socket_operation_failed, last_socket_error_as_string());
  return ntohs(sin.sin_port);
}

bool reuseaddr(socket x, bool new_value) {
  int on = new_value ? 1 : 0;
  auto res = setsockopt(x.id, SOL_SOCKET, SO_REUSEADDR,
                        reinterpret_cast<const void*>(&on),
                        static_cast<unsigned>(sizeof(on)));
  return res == 0;
}

tcp_stream_socket accept(tcp_accept_socket sock) {
  sockaddr_in cli = {};
  socklen_t len = sizeof(sockaddr_in);
  return tcp_stream_socket{
    ::accept(sock.id, reinterpret_cast<sockaddr*>(&cli), &len)};
}

} // namespace net
