/**
 *  @author    Jakob Otto
 *  @file      pollset_updater.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/detail/pollset_updater.hpp"

#include "net/event_result.hpp"
#include "net/operation.hpp"
#include "net/socket/pipe_socket.hpp"

#include "util/binary_serializer.hpp"
#include "util/byte_buffer.hpp"
#include "util/config.hpp"
#include "util/error.hpp"
#include "util/error_or.hpp"

#include "multiplexer_mock.hpp"
#include "net_test.hpp"

using namespace net;

namespace {

struct dummy_multiplexer : public multiplexer_mock {
  void handle_error(const util::error& err) override { last_error = err; }

  void add(detail::manager_base_ptr mgr, operation initial) override {
    add_called = true;
    last_manager = std::move(mgr);
    initial_operation = initial;
  }

  void shutdown() override { shutdown_called = true; }

  util::error last_error;
  bool shutdown_called{false};
  bool add_called{false};
  operation initial_operation{operation::none};
  detail::manager_base_ptr last_manager;
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
  detail::pollset_updater updater{pipe_reader, &mpx};
  EXPECT_EQ(updater.init(util::config{}), util::none);
}

TEST_F(pollset_updater_test, handle_shutdown) {
  detail::pollset_updater updater{pipe_reader, &mpx};
  EXPECT_EQ(updater.init(util::config{}), util::none);
  write_to_pipe(detail::pollset_updater::opcode::shutdown);
  updater.handle_read_event();
  EXPECT_EQ(mpx.last_error, util::none);
  EXPECT_TRUE(mpx.shutdown_called);
}

TEST_F(pollset_updater_test, handle_add) {
  detail::pollset_updater updater{pipe_reader, &mpx};
  EXPECT_EQ(updater.init(util::config{}), util::none);
  auto mgr = util::make_intrusive<detail::event_handler>(invalid_socket, &mpx);
  mgr->ref();
  EXPECT_EQ(mgr->ref_count(), 2);
  write_to_pipe(detail::pollset_updater::opcode::add, mgr.get(),
                operation::read);
  updater.handle_read_event();
  EXPECT_EQ(mgr->ref_count(), 2);
  EXPECT_EQ(mpx.last_error, util::none);
  EXPECT_TRUE(mpx.add_called);
  EXPECT_EQ(mpx.last_manager, mgr);
  mpx.last_manager.reset();
  EXPECT_EQ(mgr->ref_count(), 1);
  EXPECT_EQ(mpx.initial_operation, operation::read);
}

TEST_F(pollset_updater_test, handle_write_event) {
  detail::pollset_updater updater{pipe_reader, &mpx};
  EXPECT_EQ(updater.handle_write_event(), event_result::error);
}

TEST_F(pollset_updater_test, handle_timeout) {
  detail::pollset_updater updater{pipe_reader, &mpx};
  EXPECT_EQ(updater.handle_timeout(42), event_result::error);
}
