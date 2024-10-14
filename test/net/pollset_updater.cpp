/**
 *  @author    Jakob Otto
 *  @file      pollset_updater.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/pollset_updater.hpp"

#include "net/event_result.hpp"
#include "net/multiplexer.hpp"
#include "net/operation.hpp"
#include "net/socket/pipe_socket.hpp"
#include "net/socket_manager.hpp"
#include "net/socket_manager_factory.hpp"

#include "util/binary_serializer.hpp"
#include "util/byte_buffer.hpp"
#include "util/config.hpp"
#include "util/error.hpp"

#include "net_test.hpp"

using namespace net;

namespace {

struct dummy_multiplexer : public multiplexer {
  util::error init(socket_manager_factory_ptr, const util::config&) override {
    return util::none;
  }

  void start() override {}

  void shutdown() override { shutdown_called = true; }

  void join() override {}

  bool running() const override { return false; }

  void handle_error(const util::error& err) override { last_error = err; }

  util::error poll_once(bool) override { return util::none; }

  void add(socket_manager_ptr mgr, operation initial) override {
    add_called = true;
    last_manager = mgr.get();
    initial_operation = initial;
  }

  void enable(socket_manager_ptr, operation) override {}

  void disable(socket_manager_ptr, operation, bool) override {}

  uint64_t set_timeout(socket_manager_ptr,
                       std::chrono::system_clock::time_point) override {
    return 0;
  }

  util::error last_error;
  bool shutdown_called{false};
  bool add_called{false};
  operation initial_operation{operation::none};
  socket_manager* last_manager{nullptr};
};

// Implements all pure virtual functions from the socket_manager class
class dummy_manager : public socket_manager {
public:
  dummy_manager(net::socket handle, multiplexer* mpx)
    : socket_manager{handle, mpx} {
    // nop
  }

  util::error init(const util::config&) override { return util::none; }

  event_result handle_read_event() override { return event_result::done; }

  event_result handle_write_event() override { return event_result::done; }

  event_result handle_timeout(uint64_t) override { return event_result::done; }
};

struct pollset_updater_test : public ::testing::Test, public dummy_multiplexer {
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
};

} // namespace

TEST_F(pollset_updater_test, init) {
  pollset_updater updater{pipe_reader, this};
  EXPECT_EQ(updater.init(util::config{}), util::none);
}

TEST_F(pollset_updater_test, handle_shutdown) {
  pollset_updater updater{pipe_reader, this};
  EXPECT_EQ(updater.init(util::config{}), util::none);
  write_to_pipe(pollset_updater::shutdown_code);
  updater.handle_read_event();
  EXPECT_EQ(last_error, util::none);
  EXPECT_TRUE(shutdown_called);
}

TEST_F(pollset_updater_test, handle_add) {
  pollset_updater updater{pipe_reader, this};
  EXPECT_EQ(updater.init(util::config{}), util::none);
  auto mgr = util::make_intrusive<dummy_manager>(invalid_socket, this);
  mgr->ref();
  EXPECT_EQ(mgr->ref_count(), 2);
  write_to_pipe(pollset_updater::add_code, mgr.get(), operation::read);
  updater.handle_read_event();
  EXPECT_EQ(mgr->ref_count(), 1);
  EXPECT_EQ(last_error, util::none);
  EXPECT_TRUE(add_called);
  EXPECT_EQ(last_manager, mgr.get());
  EXPECT_EQ(initial_operation, operation::read);
}

TEST_F(pollset_updater_test, handle_write_event) {
  pollset_updater updater{pipe_reader, this};
  EXPECT_EQ(updater.init(util::config{}), util::none);
  EXPECT_EQ(updater.handle_write_event(), event_result::error);
  EXPECT_EQ(last_error.code(), util::error_code::runtime_error);
}

TEST_F(pollset_updater_test, handle_timeout) {
  pollset_updater updater{pipe_reader, this};
  EXPECT_EQ(updater.init(util::config{}), util::none);
  EXPECT_EQ(updater.handle_timeout(42), event_result::error);
  EXPECT_EQ(last_error.code(), util::error_code::runtime_error);
}
