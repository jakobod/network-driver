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
  std::vector<std::thread> readers;
  util::scope_guard guard([&]() {
    mpx->shutdown();
    mpx->join();
    for (auto& t : readers)
      t.join();
  });
  auto epoll_mpx_ptr = std::dynamic_pointer_cast<epoll_multiplexer>(mpx);
  auto sockets = connect_to_mpx(mpx->port(), num_clients);
  util::byte_array<128> write_buf;
  fill(write_buf);
  for (auto& sock : sockets)
    readers.emplace_back([=]() {
      util::byte_array<128> buf;
      for (size_t i = 0; i < num_rounds; ++i) {
        ASSERT_EQ(read(sock, buf), buf.size());
        EXPECT_TRUE(
          std::equal(write_buf.begin(), write_buf.end(), buf.begin()));
      }
    });
  for (size_t round = 0; round < num_rounds; ++round) {
    for (auto& sock : sockets)
      write(sock, write_buf);
  }
}
