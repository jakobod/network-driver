/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 25.03.2021
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <variant>

#include "benchmark/result.hpp"
#include "benchmark/socket_manager_impl.hpp"
#include "benchmark/tcp_stream_writer.hpp"
#include "detail/error.hpp"
#include "detail/socket_guard.hpp"
#include "net/multiplexer.hpp"
#include "net/socket.hpp"
#include "net/socket_manager_factory.hpp"
#include "net/stream_socket.hpp"
#include "net/tcp_accept_socket.hpp"
#include "net/tcp_stream_socket.hpp"

struct dummy_factory : public net::socket_manager_factory {
  dummy_factory(benchmark::result_ptr results) : results_(std::move(results)) {
    // nop
  }

  net::socket_manager_ptr make(net::socket handle,
                               net::multiplexer* mpx) override {
    std::cout << "factory created new socket manager" << std::endl;
    return std::make_shared<benchmark::socket_manager_impl>(handle, mpx,
                                                            results_);
  }

private:
  benchmark::result_ptr results_;
};

template <class What>
[[noreturn]] void exit(What what) {
  std::cerr << what << std::endl;
  abort();
}

void run_server() {
  using namespace std::chrono;
  auto results = std::make_shared<benchmark::result>();
  net::multiplexer mpx(results);
  auto factory = std::make_shared<dummy_factory>(results);
  if (auto err = mpx.init(std::move(factory)))
    exit(err);
  mpx.start();

  bool running = true;
  auto printer = [&]() {
    while (running) {
      auto now = system_clock::now();
      std::this_thread::sleep_until(now + seconds(1));
      std::cout << *results << std::endl;
    }
  };
  std::thread printer_t(printer);

  std::cout << "waiting for user input" << std::endl;
  std::string dummy;
  std::getline(std::cin, dummy);

  std::cout << "shutting down the multiplexer" << std::endl;
  mpx.shutdown();
  std::cout << "joining now!" << std::endl;
  mpx.join();
  std::cout << "Done and joined. BYEEEEE!" << std::endl;
  running = false;
  printer_t.join();
}

void run_client(std::string host, uint16_t port, size_t amount) {
  benchmark::tcp_stream_writer writer(amount);
  if (auto err = writer.init(host, port))
    exit(err);
  writer.start();
  writer.join();
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