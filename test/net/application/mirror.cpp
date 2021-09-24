/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>

#include "fwd.hpp"
#include "net/application/mirror.hpp"
#include "net/epoll_multiplexer.hpp"
#include "net/error.hpp"
#include "net/manager/stream.hpp"
#include "net/multiplexer.hpp"
#include "net/multithreaded_epoll_multiplexer.hpp"

using namespace net;
using namespace std::chrono_literals;

namespace {

struct dummy_mgr {
  void register_writing() {
    registered = true;
  }

  void configure_next_read(receive_policy) {
    configured = true;
  }

  util::byte_buffer& write_buffer() {
    return buf;
  }

  bool configured = false;
  bool registered = false;
  util::byte_buffer buf;
};

template <class Container>
void fill(Container& container) {
  uint8_t b = 0;
  for (auto& v : container)
    v = std::byte(b++);
}

multiplexer_ptr make_simple_server() {
  auto fact = std::make_shared<manager::stream_factory<application::mirror>>();
  auto mpx_res = make_epoll_multiplexer(std::move(fact));
  EXPECT_EQ(get_error(mpx_res), nullptr);
  auto mpx = std::get<net::multiplexer_ptr>(mpx_res);
  mpx->start();
  return mpx;
}

multiplexer_ptr make_multithreaded_server(size_t num_threads) {
  auto fact = std::make_shared<manager::stream_factory<application::mirror>>();
  auto mpx_res = make_multithreaded_epoll_multiplexer(std::move(fact),
                                                      num_threads);
  EXPECT_EQ(get_error(mpx_res), nullptr);
  auto mpx = std::get<net::multiplexer_ptr>(mpx_res);
  mpx->start();
  return mpx;
}

std::vector<tcp_stream_socket> connect_to_mpx(uint16_t port,
                                              size_t num_clients) {
  std::vector<tcp_stream_socket> sockets;
  for (size_t i = 0; i < num_clients; ++i) {
    auto conn_res = make_connected_tcp_stream_socket("127.0.0.1", port);
    EXPECT_EQ(get_error(conn_res), nullptr);
    sockets.emplace_back(std::get<tcp_stream_socket>(conn_res));
  }
  return sockets;
}

std::vector<std::thread>
start_readers(const std::vector<tcp_stream_socket>& sockets, size_t num_rounds,
              util::const_byte_span expected) {
  std::vector<std::thread> threads;
  for (auto& sock : sockets)
    threads.emplace_back([=]() {
      util::byte_array<128> buf;
      for (size_t i = 0; i < num_rounds; ++i) {
        ASSERT_EQ(read(sock, buf), buf.size());
        EXPECT_TRUE(std::equal(expected.begin(), expected.end(), buf.begin()));
      }
    });
  return threads;
}

bool await_all_accepted(multiplexer_ptr mpx, size_t expected) {
  size_t round = 0;
  while (++round < 10 && mpx->num_socket_managers() < expected)
    std::this_thread::sleep_for(10ms);
  return mpx->num_socket_managers() == expected;
}

} // namespace

TEST(mirror_test, consume_produce) {
  dummy_mgr mgr;
  application::mirror mirror;
  util::byte_array<1024> buf;
  fill(buf);
  EXPECT_TRUE(mirror.consume(mgr, buf));
  EXPECT_TRUE(mirror.produce(mgr));
  EXPECT_FALSE(mirror.produce(mgr));
  EXPECT_TRUE(mgr.configured);
  EXPECT_TRUE(mgr.registered);
  EXPECT_TRUE(std::equal(buf.begin(), buf.end(), mgr.buf.begin()));
}

TEST(mirror_test, epoll_mpx) {
  static constexpr size_t num_clients = 1;
  static constexpr size_t num_rounds = 10;
  auto mpx = make_simple_server();
  const auto expected_num_mgrs = mpx->num_socket_managers() + num_clients;
  auto sockets = connect_to_mpx(mpx->port(), num_clients);
  await_all_accepted(mpx, expected_num_mgrs);
  util::byte_array<128> write_buf;
  fill(write_buf);
  auto readers = start_readers(sockets, num_rounds, write_buf);
  for (size_t round = 0; round < num_rounds; ++round) {
    for (auto& sock : sockets)
      ASSERT_EQ(write(sock, write_buf), write_buf.size());
  }
  // Shutdown
  for (auto& t : readers)
    t.join();
  mpx->shutdown();
  mpx->join();
}

TEST(mirror_test, multithreaded_epoll_mpx) {
  static constexpr size_t num_threads = 1;
  static constexpr size_t num_clients = 1;
  static constexpr size_t num_rounds = 10;
  auto mpx = make_multithreaded_server(num_threads);
  const auto expected_num_mgrs = mpx->num_socket_managers() + num_clients;
  auto sockets = connect_to_mpx(mpx->port(), num_clients);
  await_all_accepted(mpx, expected_num_mgrs);
  util::byte_array<128> write_buf;
  fill(write_buf);
  auto readers = start_readers(sockets, num_rounds, write_buf);
  for (size_t round = 0; round < num_rounds; ++round) {
    for (auto& sock : sockets)
      ASSERT_EQ(write(sock, write_buf), write_buf.size());
  }
  // Shutdown
  std::cerr << "joining" << std::endl;
  for (auto& t : readers)
    t.join();
  mpx->shutdown();
  mpx->join();
}
