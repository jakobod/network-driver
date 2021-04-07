/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 25.03.2021
 */

#include <iostream>
#include <string>
#include <variant>

#include "detail/error.hpp"
#include "detail/socket_guard.hpp"
#include "net/multiplexer.hpp"
#include "net/socket.hpp"
#include "net/socket_manager_factory.hpp"
#include "net/socket_manager_impl.hpp"
#include "net/stream_socket.hpp"
#include "net/tcp_accept_socket.hpp"
#include "net/tcp_stream_socket.hpp"

struct dummy_factory : public net::socket_manager_factory {
  net::socket_manager_ptr make(net::socket handle,
                               net::multiplexer* mpx) override {
    std::cout << "factory created new socket manager" << std::endl;
    return std::make_shared<net::socket_manager_impl>(handle, mpx, true);
  }
};

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

void test_mpx() {
  net::multiplexer mpx;
  auto factory = std::make_shared<dummy_factory>();
  if (auto err = mpx.init(std::move(factory)))
    exit(err);
  mpx.start();

  std::cout << "waiting for user input" << std::endl;
  std::string dummy;
  std::getline(std::cin, dummy);

  std::cout << "shutting down the multiplexer" << std::endl;
  mpx.shutdown();
  std::cout << "joining now!" << std::endl;
  mpx.join();
  std::cout << "Done and joined. BYEEEEE!" << std::endl;
}

int main() {
  test_mpx();
}
