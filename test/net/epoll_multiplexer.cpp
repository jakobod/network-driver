/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include <gtest/gtest.h>

#include "fwd.hpp"
#include "net/epoll_multiplexer.hpp"
#include "net/error.hpp"
#include "net/socket_manager_factory.hpp"
#include "net/tcp_stream_socket.hpp"

using namespace net;

namespace {

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
};

struct dummy_factory : public socket_manager_factory {
  socket_manager_ptr make(net::socket handle, multiplexer* mpx) override {
    return std::make_shared<dummy_socket_manager>(handle, mpx);
  }
};

struct epoll_multiplexer_test : public testing::Test {
  epoll_multiplexer_test() : factory(std::make_shared<dummy_factory>()) {
    mpx.set_thread_id();
    EXPECT_EQ(mpx.init(factory), none);
  }

  tcp_stream_socket connect_to_mpx() {
    auto sock_res = make_connected_tcp_stream_socket("127.0.0.1", mpx.port());
    EXPECT_EQ(get_error(sock_res), nullptr);
    return std::get<tcp_stream_socket>(sock_res);
  }

  socket_manager_factory_ptr factory;
  epoll_multiplexer mpx;
};

} // namespace

TEST_F(epoll_multiplexer_test, mpx_accepts_connections) {
  std::array<tcp_stream_socket, 10> sockets;
  for (auto& sock : sockets) {
    sock = connect_to_mpx();
    mpx.poll_once(false);
    std::cerr << "sock = " << sock.id << std::endl;
  }
  // EXPECT_EQ(mpx.num_socket_managers(), 12);
  // mpx.shutdown();
  // mpx.poll_once(false);
  // EXPECT_EQ(mpx.num_socket_managers(), 1);
  // EXPECT_FALSE(mpx.running());
}

TEST_F(epoll_multiplexer_test, shutdown) {
  // add_managers(10);

  // EXPECT_EQ(mpx.num_socket_managers(), 12);
  // mpx.shutdown();
  // mpx.join();
  // EXPECT_FALSE(mpx.running());
}
