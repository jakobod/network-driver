/**
 *  @author    Jakob Otto
 *  @file      acceptor.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/acceptor.hpp"

#include "net/event_result.hpp"
#include "net/manager_base.hpp"
#include "net/multiplexer_base.hpp"

#include "net/ip/v4_address.hpp"
#include "net/ip/v4_endpoint.hpp"

#include "net/socket/tcp_accept_socket.hpp"
#include "net/socket/tcp_stream_socket.hpp"

#include "util/error.hpp"
#include "util/error_or.hpp"

#include "net_test.hpp"

using namespace net;
using namespace net::ip;

namespace {

struct dummy_multiplexer : public multiplexer_base {
  void handle_error(const util::error& err) override { last_error = err; }

  void add(manager_base_ptr new_mgr, operation) override {
    mgr = std::move(new_mgr);
  }

  void shutdown() override {
    // nop
  }

  void enable(manager_base&, operation) override {
    // nop
  }

  uint64_t set_timeout(manager_base_ptr,
                       std::chrono::steady_clock::time_point) override {
    return 0;
  }

  util::error last_error;
  manager_base_ptr mgr = nullptr;
};

struct acceptor_test : public testing::Test {
  acceptor_test() {
    auto res = make_tcp_accept_socket({net::ip::v4_address::localhost, 0});
    EXPECT_EQ(util::get_error(res), nullptr);
    auto sock_pair = std::get<acceptor_pair>(res);
    auto factory = [this](net::socket hdl, multiplexer_base* mpx) {
      factory_called = true;
      return util::make_intrusive<manager_base>(hdl, mpx);
    };
    acc = std::make_unique<acceptor>(sock_pair.first, &mpx, std::move(factory));
    port = sock_pair.second;
  }

  dummy_multiplexer mpx;
  std::unique_ptr<acceptor> acc;
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

TEST_F(acceptor_test, handle_write_event) {
  EXPECT_EQ(acc->handle_write_event(), event_result::error);
  EXPECT_FALSE(factory_called);
}

// TEST_F(acceptor_test, handle_timeout) {
//   EXPECT_EQ(acc->handle_timeout(0), event_result::error);
//   EXPECT_FALSE(factory_called);
// }
