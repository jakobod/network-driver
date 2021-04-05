/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 25.03.2021
 */

#include <iostream>
#include <type_traits>

#include "detail/socket_guard.hpp"
#include "net/socket.hpp"
#include "net/stream_socket.hpp"
#include "net/tcp_accept_socket.hpp"
#include "net/tcp_stream_socket.hpp"

[[noreturn]] void exit(std::string msg = "") {
  std::cerr << msg << std::endl;
  abort();
}

int main() {
  // auto accept_socket = net::make_tcp_accept_socket(12345);
  // if (accept_socket != net::invalid_socket)
  //   exit("failed to create accept socket");
  // std::cerr << "waiting for an incoming connection" << std::endl;
  // auto guard = detail::make_socket_guard(net::accept(accept_socket));
  // std::cerr << "accepted!" << std::endl;
  // if (guard == net::invalid_socket)
  //   exit("accepted invalid socket");
  // std::cerr << "all fine!" << std::endl;
  // return 0;
}
