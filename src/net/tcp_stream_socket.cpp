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

tcp_stream_socket make_connected_tcp_stream_socket(const std::string ip,
                                                   uint16_t port) {
  sockaddr_in servaddr = {};
  if (port == 0) {
    std::cerr << "port is zero" << std::endl;
    abort();
  }
  tcp_stream_socket sock{::socket(AF_INET, SOCK_STREAM, 0)};
  auto guard = detail::make_socket_guard(sock);
  if (sock == invalid_socket_id) {
    std::cerr << "socket creation failed..." << std::endl;
    abort();
  }

  // assign IP, PORT
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(ip.c_str());
  servaddr.sin_port = htons(port);

  // connect the client socket to server socket
  if (::connect(sock.id, (sockaddr*) &servaddr, sizeof(sockaddr_in)) != 0) {
    std::cerr << "connection with the server failed..." << std::endl;
    abort();
  }
  return guard.release();
}

} // namespace net
