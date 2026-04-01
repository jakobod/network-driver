/**
 *  @author    Jakob Otto
 *  @file      pollset_updater.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/pollset_updater.hpp"

#include "net/event_result.hpp"
#include "net/manager_base.hpp"
#include "net/multiplexer_base.hpp"
#include "net/operation.hpp"
#include "net/socket/pipe_socket.hpp"

#include "util/binary_serializer.hpp"
#include "util/byte_buffer.hpp"
#include "util/config.hpp"
#include "util/error.hpp"
#include "util/error_or.hpp"

#include "net_test.hpp"

using namespace net;

namespace {

struct dummy_multiplexer : public multiplexer_base {
  void handle_error(const util::error& err) override { last_error = err; }

  void add(manager_base_ptr mgr, operation initial) override {
    add_called = true;
    last_manager = mgr.get();
    initial_operation = initial;
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
  bool shutdown_called{false};
  bool add_called{false};
  operation initial_operation{operation::none};
  manager_base* last_manager{nullptr};
};

struct pollset_updater_test : public ::testing::Test {
  pollset_updater_test() {
    auto pipe_res = make_pipe();
    EXPECT_EQ(util::get_error(pipe_res), nullptr);
    auto [reader, writer] = std::get<pipe_socket_pair>(pipe_res);
    pipe_reader = reader;
    pipe_writer = writer;
  }

  template <class... Ts>
  void write_to_pipe(Ts&&... ts) {
    util::byte_buffer buf;
    util::binary_serializer sink{buf};
    sink(std::forward<Ts>(ts)...);
    EXPECT_EQ(write(pipe_writer, buf), static_cast<ptrdiff_t>(buf.size()));
  }

  pipe_socket pipe_reader;
  pipe_socket pipe_writer;
  dummy_multiplexer mpx;
};

} // namespace

TEST_F(pollset_updater_test, init) {
  pollset_updater updater{pipe_reader, &mpx};
  EXPECT_EQ(updater.init(util::config{}), util::none);
}

TEST_F(pollset_updater_test, handle_shutdown) {
  pollset_updater updater{pipe_reader, &mpx};
  EXPECT_EQ(updater.init(util::config{}), util::none);
  write_to_pipe(pollset_updater::shutdown_code);
  updater.handle_read_event();
  EXPECT_EQ(mpx.last_error, util::none);
  EXPECT_TRUE(mpx.shutdown_called);
}

TEST_F(pollset_updater_test, handle_add) {
  pollset_updater updater{pipe_reader, &mpx};
  EXPECT_EQ(updater.init(util::config{}), util::none);
  auto mgr = util::make_intrusive<manager_base>(invalid_socket, &mpx);
  mgr->ref();
  EXPECT_EQ(mgr->ref_count(), 2);
  write_to_pipe(pollset_updater::add_code, mgr.get(), operation::read);
  updater.handle_read_event();
  EXPECT_EQ(mgr->ref_count(), 1);
  EXPECT_EQ(mpx.last_error, util::none);
  EXPECT_TRUE(mpx.add_called);
  EXPECT_EQ(mpx.last_manager, mgr.get());
  EXPECT_EQ(mpx.initial_operation, operation::read);
}

TEST_F(pollset_updater_test, handle_write_event) {
  pollset_updater updater{pipe_reader, &mpx};
  EXPECT_EQ(updater.handle_write_event(), event_result::error);
}

TEST_F(pollset_updater_test, handle_timeout) {
  pollset_updater updater{pipe_reader, &mpx};
  EXPECT_EQ(updater.handle_timeout(42), event_result::error);
}
