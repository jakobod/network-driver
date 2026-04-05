/**
 *  @author    Jakob Otto
 *  @file      acceptor.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/detail/acceptor.hpp"

#include "net/event_result.hpp"
#include "net/operation.hpp"

#include "net/detail/event_handler.hpp"

#include "net/ip/v4_address.hpp"
#include "net/ip/v4_endpoint.hpp"

#include "net/socket/tcp_accept_socket.hpp"
#include "net/socket/tcp_stream_socket.hpp"

#include "util/error.hpp"
#include "util/error_or.hpp"

#include "multiplexer_mock.hpp"
#include "net_test.hpp"

using namespace net;
using namespace net::ip;

namespace {

struct dummy_multiplexer : public multiplexer_mock {
  void add(detail::manager_base_ptr new_mgr, operation) override {
    mgr = std::move(new_mgr);
  }

  void handle_error(const util::error& err) override { last_error = err; }

  util::error last_error;
  detail::manager_base_ptr mgr;
};

struct acceptor_test : public testing::Test {
  acceptor_test() {
    auto sock_pair = UNPACK_EXPRESSION(make_tcp_accept_socket({net::ip::v4_address::localhost, 0}));
    auto factory = [this](net::socket handle) -> detail::manager_base_ptr {
      factory_called = true;
      return util::make_intrusive<detail::event_handler>(handle, &mpx);
    };

    acc = std::make_unique<detail::acceptor<detail::event_handler>>(
      sock_pair.first, &mpx, std::move(factory));
    port = sock_pair.second;
  }

  dummy_multiplexer mpx;
  std::unique_ptr<detail::acceptor<detail::event_handler>> acc;
  uint16_t port{0};

  // Test check flags
  bool factory_called{false};
};

} // namespace

TEST_F(acceptor_test, handle_read_event) {
  const v4_endpoint ep{v4_address::localhost, port};
  auto sock = make_connected_tcp_stream_socket(ep);
  EXPECT_EQ(acc->handle_read_event(), event_result::ok);
  EXPECT_EQ(mpx.last_error, util::none);
  EXPECT_NE(mpx.mgr, nullptr);
  EXPECT_TRUE(factory_called);
}
