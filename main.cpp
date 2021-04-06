/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 25.03.2021
 */

#include <iostream>
#include <variant>

#include "detail/error.hpp"
#include "detail/socket_guard.hpp"
#include "net/socket.hpp"
#include "net/stream_socket.hpp"
#include "net/tcp_accept_socket.hpp"
#include "net/tcp_stream_socket.hpp"

[[noreturn]] void exit(std::string msg = "") {
  std::cerr << msg << std::endl;
  abort();
}

[[noreturn]] void exit(detail::error err) {
  std::cerr << err << std::endl;
  abort();
}

void test_connect() {
  auto res = net::make_tcp_accept_socket(0);
  if (auto err = std::get_if<detail::error>(&res))
    exit(*err);
  auto accept_socket_pair
    = std::get<std::pair<net::tcp_accept_socket, uint16_t>>(res);
  auto accept_socket = accept_socket_pair.first;
  std::cerr << "socket is bound to port: " << accept_socket_pair.second
            << std::endl;
  if (accept_socket == net::invalid_socket)
    exit("failed to create accept socket");
  std::cerr << "waiting for an incoming connection" << std::endl;
  auto guard = detail::make_socket_guard(net::accept(accept_socket));
  std::cerr << "accepted!" << std::endl;
  if (guard == net::invalid_socket)
    exit("accepted invalid socket");
  std::cerr << "all fine!" << std::endl;
}

int main() {
  test_connect();
}
