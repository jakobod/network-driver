/**
 *  @author    Jakob Otto
 *  @file      acceptor.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/acceptor.hpp"
#include "net/event_result.hpp"
#include "net/ip/v4_address.hpp"
#include "net/ip/v4_endpoint.hpp"
#include "net/multiplexer.hpp"
#include "net/socket/tcp_accept_socket.hpp"
#include "net/socket/tcp_stream_socket.hpp"
#include "net/socket_manager_factory.hpp"

#include "util/config.hpp"
#include "util/error.hpp"
#include "util/intrusive_ptr.hpp"

#include "net_test.hpp"

using namespace net;
using namespace net::ip;

namespace {

struct dummy_multiplexer : public multiplexer {
  util::error init(socket_manager_factory_ptr, const util::config&) override {
    return util::none;
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

  bool running() const override { return false; }

  void handle_error(const util::error& err) override { last_error = err; }

  util::error poll_once(bool) override { return util::none; }

  void add(socket_manager_ptr new_mgr, operation) override {
    mgr = std::move(new_mgr);
  }

  void enable(socket_manager_ptr, operation) override {
    // nop
  }

  void disable(socket_manager_ptr, operation, bool) override {
    // nop
  }

  uint64_t set_timeout(socket_manager_ptr,
                       std::chrono::system_clock::time_point) override {
    return 0;
  }

  util::error last_error;
  socket_manager_ptr mgr = nullptr;
};

struct dummy_socket_manager : public socket_manager {
  dummy_socket_manager(net::socket handle, multiplexer* parent)
    : socket_manager(handle, parent) {
    // nop
  }

  util::error init(const util::config&) override { return util::none; }

  event_result handle_read_event() override { return event_result::done; }

  event_result handle_write_event() override { return event_result::done; }

  event_result handle_timeout(uint64_t) override { return event_result::done; }
};

struct dummy_factory : socket_manager_factory {
  ~dummy_factory() override = default;

  socket_manager_ptr make(net::socket hdl, multiplexer* mpx) override {
    return util::make_intrusive<dummy_socket_manager>(hdl, mpx);
  };
};

struct acceptor_test : public testing::Test {
  acceptor_test() {
    auto res = make_tcp_accept_socket({net::ip::v4_address::localhost, 0});
    EXPECT_EQ(get_error(res), nullptr);
    auto sock_pair = std::get<acceptor_pair>(res);
    acc = std::make_unique<acceptor>(sock_pair.first, &mpx,
                                     std::make_shared<dummy_factory>());
    port = sock_pair.second;
  }

  dummy_multiplexer mpx;
  std::unique_ptr<acceptor> acc;
  uint16_t port;
};

} // namespace

TEST_F(acceptor_test, handle_read_event) {
  const v4_endpoint ep{v4_address::localhost, port};
  auto sock = make_connected_tcp_stream_socket(ep);
  EXPECT_EQ(acc->handle_read_event(), event_result::ok);
  EXPECT_EQ(mpx.last_error, util::none);
  EXPECT_NE(mpx.mgr, nullptr);
}

TEST_F(acceptor_test, handle_write_event) {
  EXPECT_EQ(acc->handle_write_event(), event_result::error);
  EXPECT_EQ(mpx.last_error, util::error(util::error_code::runtime_error));
}
