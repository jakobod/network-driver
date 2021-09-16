/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include <gtest/gtest.h>

#include "net/acceptor.hpp"
#include "net/error.hpp"
#include "net/multiplexer.hpp"
#include "net/socket_manager_factory.hpp"
#include "net/tcp_accept_socket.hpp"
#include "net/tcp_stream_socket.hpp"

using namespace net;

namespace {

struct dummy_multiplexer : public multiplexer {
  error init(socket_manager_factory_ptr, uint16_t) override {
    return none;
  }

  void start() override {
    // nop
  }

  void shutdown() override {
    // nop
  }

  void join() override {
    // nop
  }

  bool running() override {
    return false;
  }

  void handle_error(error err) override {
    last_error = std::move(err);
  }

  error poll_once(bool) override {
    return none;
  }

  void add(socket_manager_ptr new_mgr, operation) override {
    mgr = std::move(new_mgr);
  }

  void enable(socket_manager&, operation) override {
    // nop
  }

  void disable(socket_manager&, operation, bool) override {
    // nop
  }

  void set_timeout(socket_manager*, uint64_t,
                   std::chrono::system_clock::time_point) override {
    // nop
  }

  error last_error;
  socket_manager_ptr mgr = nullptr;
};

struct dummy_socket_manager : public socket_manager {
  dummy_socket_manager(net::socket handle, multiplexer* parent)
    : socket_manager(handle, parent) {
    // nop
  }

  bool handle_read_event() override {
    return false;
  }

  bool handle_write_event() override {
    return false;
  }

  bool handle_timeout(uint64_t) override {
    return false;
  }
};

struct dummy_factory : socket_manager_factory {
  socket_manager_ptr make(net::socket hdl, multiplexer* mpx) {
    return std::make_shared<dummy_socket_manager>(hdl, mpx);
  };
};

struct acceptor_test : public testing::Test {
  acceptor_test() : acc{tcp_accept_socket{0}, nullptr, nullptr} {
    auto res = make_tcp_accept_socket(0);
    EXPECT_EQ(get_error(res), nullptr);
    auto sock_pair = std::get<acceptor_pair>(res);
    acc = std::move(
      acceptor(sock_pair.first, &mpx, std::make_shared<dummy_factory>()));
    port = sock_pair.second;
  }

  dummy_multiplexer mpx;
  acceptor acc;
  uint16_t port;
};

} // namespace

TEST_F(acceptor_test, handle_read_event) {
  auto sock = make_connected_tcp_stream_socket("127.0.0.1", port);
  EXPECT_TRUE(acc.handle_read_event());
  EXPECT_EQ(mpx.last_error, none);
  EXPECT_NE(mpx.mgr, nullptr);
}

TEST_F(acceptor_test, handle_write_event) {
  EXPECT_FALSE(acc.handle_write_event());
  EXPECT_EQ(mpx.last_error, error(runtime_error));
}
