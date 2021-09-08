/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include <gtest/gtest.h>

#include "detail/error.hpp"
#include "net/acceptor.hpp"
#include "net/multiplexer.hpp"
#include "net/socket_manager_factory.hpp"
#include "net/tcp_accept_socket.hpp"
#include "net/tcp_stream_socket.hpp"

namespace {

struct dummy_multiplexer : public net::multiplexer {
  detail::error init(net::socket_manager_factory_ptr) override {
    return detail::none;
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

  void handle_error(detail::error err) override {
    last_error = std::move(err);
  }

  void add(net::socket_manager_ptr new_mgr, net::operation) override {
    mgr = std::move(new_mgr);
  }

  void enable(net::socket_manager&, net::operation) override {
    // nop
  }

  void disable(net::socket_manager&, net::operation, bool) override {
    // nop
  }

  detail::error last_error = detail::none;
  net::socket_manager_ptr mgr = nullptr;
};

struct dummy_socket_manager : public net::socket_manager {
  dummy_socket_manager(net::socket handle, net::multiplexer* parent)
    : net::socket_manager(handle, parent) {
    // nop
  }

  bool handle_read_event() override {
    return false;
  }

  bool handle_write_event() override {
    return false;
  }
};

struct dummy_factory : net::socket_manager_factory {
  net::socket_manager_ptr make(net::socket hdl, net::multiplexer* mpx) {
    std::cerr << "Creating new dummy_socket_manager with id = " << hdl.id
              << std::endl;
    return std::make_shared<dummy_socket_manager>(hdl, mpx);
  };
};

struct acceptor_test : public testing::Test {
  acceptor_test() : acc{net::tcp_accept_socket{0}, nullptr, nullptr} {
    auto res = net::make_tcp_accept_socket(0);
    auto err = std::get_if<detail::error>(&res);
    EXPECT_EQ(err, nullptr);
    auto sock_pair = std::get<net::acceptor_pair>(res);
    acc = std::move(
      net::acceptor(sock_pair.first, &mpx, std::make_shared<dummy_factory>()));
    port = sock_pair.second;
  }

  dummy_multiplexer mpx;
  net::acceptor acc;
  uint16_t port;
};

} // namespace

TEST_F(acceptor_test, handle_read_event) {
  auto sock = net::make_connected_tcp_stream_socket("127.0.0.1", port);
  EXPECT_TRUE(acc.handle_read_event());
  EXPECT_EQ(mpx.last_error, detail::none);
  EXPECT_NE(mpx.mgr, nullptr);
}

TEST_F(acceptor_test, handle_write_event) {
  EXPECT_FALSE(acc.handle_write_event());
  EXPECT_EQ(mpx.last_error, detail::error(detail::runtime_error));
}
