/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/acceptor.hpp"
#include "net/event_result.hpp"
#include "net/multiplexer.hpp"
#include "net/socket_manager_factory.hpp"
#include "net/tcp_accept_socket.hpp"
#include "net/tcp_stream_socket.hpp"

#include "util/error.hpp"

#include "net_test.hpp"

using namespace net;

namespace {

struct dummy_multiplexer : public multiplexer {
  util::error init(socket_manager_factory_ptr, uint16_t) override {
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

  bool running() override {
    return false;
  }

  void handle_error(util::error err) override {
    last_error = std::move(err);
  }

  util::error poll_once(bool) override {
    return util::none;
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

  void set_timeout(socket_manager&, uint64_t,
                   std::chrono::system_clock::time_point) override {
    // nop
  }

  util::error last_error;
  socket_manager_ptr mgr = nullptr;
};

struct dummy_socket_manager : public socket_manager {
  dummy_socket_manager(net::socket handle, multiplexer* parent)
    : socket_manager(handle, parent) {
    // nop
  }

  util::error init() override {
    return util::none;
  }

  event_result handle_read_event() override {
    return event_result::done;
  }

  event_result handle_write_event() override {
    return event_result::done;
  }

  event_result handle_timeout(uint64_t) override {
    return event_result::done;
  }
};

struct dummy_factory : socket_manager_factory {
  ~dummy_factory() override = default;

  socket_manager_ptr make(net::socket hdl, multiplexer* mpx) override {
    return std::make_shared<dummy_socket_manager>(hdl, mpx);
  };
};

struct acceptor_test : public testing::Test {
  acceptor_test() {
    auto res = make_tcp_accept_socket(0);
    EXPECT_EQ(get_error(res), nullptr);
    auto sock_pair = std::get<acceptor_pair>(res);
    acc = std::make_unique<acceptor>(sock_pair.first, &mpx, std::make_shared<dummy_factory>());
    port = sock_pair.second;
  }

  dummy_multiplexer mpx;
  std::unique_ptr<acceptor> acc;
  uint16_t port;
};

} // namespace

TEST_F(acceptor_test, handle_read_event) {
  auto sock = make_connected_tcp_stream_socket("127.0.0.1", port);
  EXPECT_EQ(acc->handle_read_event(), event_result::ok);
  EXPECT_EQ(mpx.last_error, util::none);
  EXPECT_NE(mpx.mgr, nullptr);
}

TEST_F(acceptor_test, handle_write_event) {
  EXPECT_EQ(acc->handle_write_event(), event_result::error);
  EXPECT_EQ(mpx.last_error, util::error(util::error_code::runtime_error));
}
