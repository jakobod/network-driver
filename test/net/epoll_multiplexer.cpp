/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include <gtest/gtest.h>

#include "detail/error.hpp"
#include "fwd.hpp"
#include "net/epoll_multiplexer.hpp"
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
    EXPECT_EQ(mpx.init(factory), detail::none);
  }

  void add_managers(size_t num) {
    std::vector<stream_socket> sockets;
    for (size_t i = 0; i < num; ++i) {
      auto sock_pair_res = make_stream_socket_pair();
      ASSERT_EQ(std::get_if<detail::error>(&sock_pair_res), nullptr);
      auto sp = std::get<stream_socket_pair>(sock_pair_res);
      mpx.add(std::make_shared<dummy_socket_manager>(sp.first, &mpx),
              operation::read);
    }
  }

  socket_manager_factory_ptr factory;
  epoll_multiplexer mpx;
};

} // namespace

// TEST_F(epoll_multiplexer_test, connect) {
//   auto socket_res = make_connected_tcp_stream_socket("127.0.0.1",
//                                                           mpx.port());
//   ASSERT_EQ(std::get_if<tcp_stream_socket>(&socket_res), nullptr);
//   mpx.poll_once(false);
// }

TEST_F(epoll_multiplexer_test, shutdown) {
  mpx.start();
  EXPECT_TRUE(mpx.running());
  add_managers(10);
  EXPECT_EQ(mpx.num_socket_managers(), 12);
  mpx.shutdown();
  mpx.join();
  EXPECT_FALSE(mpx.running());
}
