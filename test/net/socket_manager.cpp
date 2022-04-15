/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/socket_manager.hpp"
#include "net/multiplexer.hpp"
#include "net/stream_socket.hpp"

#include "util/byte_buffer.hpp"
#include "util/error.hpp"

#include "net_test.hpp"

#include <algorithm>
#include <cstring>
#include <numeric>

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

  bool running() const override {
    return false;
  }

  void handle_error(const util::error& err) override {
    last_handled_error_ = err;
  }

  util::error poll_once(bool) override {
    return util::none;
  }

  void add(socket_manager_ptr, operation) override {
    // nop
  }

  void enable(socket_manager& mgr, operation op) override {
    mgr.mask_add(op);
    last_enabled_socket_ = mgr.handle();
    last_enabled_operation_ = op;
  }

  void disable(socket_manager&, operation, bool) override {
    // nop
  }

  void set_timeout(socket_manager&, uint64_t,
                   std::chrono::system_clock::time_point) override {
    // nop
  }

  void clear_last_enabled_state() {
    last_enabled_socket_ = invalid_socket;
    last_enabled_operation_ = operation::none;
  }

  util::error last_handled_error_;
  net::socket last_enabled_socket_ = invalid_socket;
  operation last_enabled_operation_ = operation::none;
};

// Implements all pure virtual functions from the socket_manager class
class dummy_manager : public socket_manager {
public:
  dummy_manager(net::socket handle, multiplexer* mpx)
    : socket_manager{handle, mpx} {
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

struct socket_manager_test : public testing::Test {
  socket_manager_test()
    : sockets{stream_socket{invalid_socket_id},
              stream_socket{invalid_socket_id}} {
    auto socket_res = make_stream_socket_pair();
    EXPECT_EQ(get_error(socket_res), nullptr);
    sockets = std::get<stream_socket_pair>(socket_res);
    EXPECT_TRUE(nonblocking(sockets.first, true));
    EXPECT_TRUE(nonblocking(sockets.second, true));
  }

  stream_socket_pair sockets;
  dummy_multiplexer mpx;
};

} // namespace

TEST_F(socket_manager_test, construction_and_destruction) {
  {
    dummy_manager mgr{sockets.first, &mpx};
    ASSERT_EQ(mgr.mpx(), &mpx);
    ASSERT_EQ(mgr.handle(), sockets.first);
    ASSERT_EQ(mgr.mask(), operation::none);
  }
  util::byte_buffer buf(1);
  ASSERT_EQ(read(sockets.second, buf), 0);
}

TEST_F(socket_manager_test, mask_operations) {
  dummy_manager mgr{sockets.first, &mpx};
  // Add single operations
  ASSERT_EQ(mgr.mask(), operation::none);
  ASSERT_TRUE(mgr.mask_add(operation::read));
  ASSERT_EQ(mgr.mask(), operation::read);
  ASSERT_TRUE(mgr.mask_add(operation::write));
  ASSERT_EQ(mgr.mask(), operation::read_write);
  ASSERT_TRUE(mgr.mask_del(operation::write));
  ASSERT_EQ(mgr.mask(), operation::read);
  ASSERT_TRUE(mgr.mask_del(operation::read));
  ASSERT_EQ(mgr.mask(), operation::none);

  // Add multiple operations
  ASSERT_TRUE(mgr.mask_add(operation::read_write));
  ASSERT_EQ(mgr.mask(), operation::read_write);
  ASSERT_TRUE(mgr.mask_del(operation::read_write));
  ASSERT_EQ(mgr.mask(), operation::none);

  // Add partial
  ASSERT_TRUE(mgr.mask_add(operation::read));
  ASSERT_EQ(mgr.mask(), operation::read);
  ASSERT_TRUE(mgr.mask_add(operation::read_write));
  ASSERT_EQ(mgr.mask(), operation::read_write);

  // Delete more than set
  ASSERT_TRUE(mgr.mask_del(operation::read));
  ASSERT_EQ(mgr.mask(), operation::write);
  ASSERT_TRUE(mgr.mask_del(operation::read_write));
  ASSERT_EQ(mgr.mask(), operation::none);
}

TEST_F(socket_manager_test, register_operations) {
  dummy_manager mgr{sockets.first, &mpx};
  mgr.register_writing();
  ASSERT_EQ(mpx.last_enabled_socket_, mgr.handle());
  ASSERT_EQ(mpx.last_enabled_operation_, operation::write);

  mpx.clear_last_enabled_state();
  mgr.register_writing();
  ASSERT_EQ(mpx.last_enabled_socket_, invalid_socket);
  ASSERT_EQ(mpx.last_enabled_operation_, operation::none);

  mpx.clear_last_enabled_state();
  mgr.register_reading();
  ASSERT_EQ(mpx.last_enabled_socket_, mgr.handle());
  ASSERT_EQ(mpx.last_enabled_operation_, operation::read);

  mpx.clear_last_enabled_state();
  mgr.register_reading();
  ASSERT_EQ(mpx.last_enabled_socket_, invalid_socket);
  ASSERT_EQ(mpx.last_enabled_operation_, operation::none);
}

TEST_F(socket_manager_test, handle_error) {
  dummy_manager mgr{sockets.first, &mpx};
  util::error err{util::error_code::runtime_error, "hello"};
  mgr.handle_error(err);
  ASSERT_EQ(mpx.last_handled_error_, err);
}
