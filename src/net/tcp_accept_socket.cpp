/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 31.03.2021
 */

#include "net/tcp_accept_socket.hpp"

#include <iostream>

#include "detail/socket_guard.hpp"
#include "net/socket_sys_includes.hpp"
#include "net/tcp_stream_socket.hpp"

namespace net {

tcp_accept_socket make_tcp_accept_socket(uint16_t port) {
  sockaddr_in servaddr = {};
  if (port == 0) {
    std::cerr << "port is zero" << std::endl;
    abort();
  }
  tcp_accept_socket sock{::socket(AF_INET, SOCK_STREAM, 0)};
  auto guard = detail::make_socket_guard(sock);
  if (sock == invalid_socket) {
    std::cerr << "socket creation failed..." << std::endl;
    abort();
  }
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);
  if ((::bind(sock.id, reinterpret_cast<sockaddr*>(&servaddr),
              sizeof(sockaddr)))
      != 0) {
    std::cerr << "socket bind failed..." << std::endl;
    abort();
  }
  if ((listen(sock.id, max_conn_backlog)) != 0) {
    std::cerr << "Listen failed..." << std::endl;
    abort();
  }
  return guard.release();
}

tcp_stream_socket accept(tcp_accept_socket sock) {
  sockaddr_in cli = {};
  socklen_t len = sizeof(sockaddr_in);
  return tcp_stream_socket{
    ::accept(sock.id, reinterpret_cast<sockaddr*>(&cli), &len)};
}

} // namespace net
