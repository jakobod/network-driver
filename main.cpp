/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 25.03.2021
 */

#include <iostream>
#include <string>
#include <variant>

#include "benchmark/tcp_stream_writer.hpp"
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

template <class What>
[[noreturn]] void exit(What what) {
  std::cerr << what << std::endl;
  abort();
}

void test_connect() {
  auto res = net::make_tcp_accept_socket(0);
  if (auto err = std::get_if<detail::error>(&res))
    exit(*err);
  auto accept_socket_pair = std::get<net::acceptor_pair>(res);
  auto accept_socket_guard
    = detail::make_socket_guard(accept_socket_pair.first);
  std::cerr << "socket is bound to port: " << accept_socket_pair.second
            << std::endl;
  if (accept_socket_guard == net::invalid_socket)
    exit("failed to create accept socket");
  std::cerr << "waiting for an incoming connection" << std::endl;
  auto guard = detail::make_socket_guard(net::accept(*accept_socket_guard));
  std::cerr << "accepted!" << std::endl;
  if (guard == net::invalid_socket)
    exit("accepted invalid socket");
  std::cerr << "all fine!" << std::endl;
}

void run_server() {
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

void run_client(std::string host, uint16_t port, size_t amount) {
  // auto res = net::make_tcp_accept_socket();
  // if (auto err = std::get_if<detail::error>(&res))
  //   exit(*err);
  // auto accept_socket_pair = std::get<net::acceptor_pair>(res);
  // auto acceptor_guard = detail::make_socket_guard(accept_socket_pair.first);
  // std::cerr << "Acceptor listening on port: " << accept_socket_pair.second
  //           << " fd = " << (*acceptor_guard).id << std::endl;
  // auto connection_res = net::make_connected_tcp_stream_socket(
  //   "127.0.0.1", accept_socket_pair.second);
  // if (auto err = std::get_if<detail::error>(&connection_res))
  //   exit(*err);
  // auto connected_guard = detail::make_socket_guard(
  //   std::get<net::tcp_stream_socket>(connection_res));
  // auto accepted_guard = detail::make_socket_guard(accept(*acceptor_guard));
  // if (accepted_guard == net::invalid_socket)
  //   exit("accept returned an invalid socket!");
  // std::cout << "Works like a charm!" << std::endl;
  benchmark::tcp_stream_writer writer(amount);
  if (auto err = writer.init(host, port))
    exit(err);
  writer.start();
  std::cout << "waiting for user input" << std::endl;
  std::string dummy;
  std::getline(std::cin, dummy);
  writer.stop();
  std::cerr << "joining now!" << std::endl;
  writer.join();
  std::cerr << "DONE!" << std::endl;
}

int main(int argc, char** argv) {
  bool is_server = true;
  std::string host = "";
  uint16_t port = 0;
  size_t amount = 0;

  for (int i = 0; i < argc; ++i) {
    switch (argv[i][1]) {
      case 's':
        is_server = true;
        break;
      case 'c':
        is_server = false;
        break;
      case 'p':
        port = std::stoi(std::string(argv[++i]));
        break;
      case 'h':
        host = std::string(argv[++i]);
        break;
      case 'a':
        amount = std::stoi(std::string(argv[++i]));
        break;
      default:
        break;
    }
  }
  if (is_server)
    run_server();
  else
    run_client(host, port, amount);
}
