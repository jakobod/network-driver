/**
 *  @author    Jakob Otto
 *  @file      pollset_updater.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/detail/pollset_updater.hpp"

#include "net/detail/event_handler.hpp"

#include "net/manager_result.hpp"
#include "net/operation.hpp"
#include "net/socket_guard.hpp"

#include "net/socket/pipe_socket.hpp"
#include "net/socket/stream_socket.hpp"

#include "util/binary_serializer.hpp"
#include "util/byte_buffer.hpp"
#include "util/config.hpp"
#include "util/error.hpp"
#include "util/error_or.hpp"

#include "net_test.hpp"

#include "net/detail/uring_manager.hpp"
using event_pollset_updater
  = net::detail::pollset_updater<net::detail::event_handler>;
#if defined(LIB_NET_URING)
#  include "net/detail/uring_manager.hpp"
#  include "net/detail/uring_multiplexer.hpp"
using uring_pollset_updater
  = net::detail::pollset_updater<net::detail::uring_manager>;
#endif

using namespace net;

namespace {

class multiplexer_mock : public detail::multiplexer_base {
public:
  MOCK_METHOD(void, add, (detail::manager_base_ptr, operation), (override));

private:
  MOCK_METHOD(manager_map::iterator, del, (manager_map::iterator), (override));

public:
  MOCK_METHOD(void, enable, (detail::manager_base&, operation), (override));
  MOCK_METHOD(void, disable, (detail::manager_base&, operation, bool),
              (override));
  MOCK_METHOD(util::error, poll_once, (bool), (override));
  MOCK_METHOD(void, handle_error, (util::error), (override));
  MOCK_METHOD(void, shutdown, (), (override));
};

struct event_pollset_updater_test : public ::testing::Test {
  event_pollset_updater_test() {
    auto [reader, writer] = UNPACK_EXPRESSION(make_pipe());
    pipe_writer = writer;
    updater = std::make_unique<event_pollset_updater>(reader, &mpx);
  }

  void SetUp() override {
    EXPECT_EQ(updater->init(util::config{}), util::none);
  }

  template <class... Ts>
  void write(pipe_socket handle, Ts&&... ts) {
    util::byte_buffer buf;
    util::binary_serializer sink{buf};
    sink(std::forward<Ts>(ts)...);
    EXPECT_EQ(net::write(handle, buf), static_cast<ptrdiff_t>(buf.size()));
  }

  socket_guard<pipe_socket> pipe_writer;
  multiplexer_mock mpx;
  std::unique_ptr<event_pollset_updater> updater;
};

} // namespace

TEST_F(event_pollset_updater_test, init) {
  EXPECT_CALL(mpx, add).Times(0);
  EXPECT_CALL(mpx, enable).Times(0);
  EXPECT_CALL(mpx, disable).Times(0);
  EXPECT_CALL(mpx, poll_once).Times(0);
  EXPECT_CALL(mpx, handle_error).Times(0);
  EXPECT_CALL(mpx, shutdown).Times(0);
  // The updater is created and initialized in the fixture
}

TEST_F(event_pollset_updater_test, handle_shutdown) {
  EXPECT_CALL(mpx, add).Times(0);
  EXPECT_CALL(mpx, enable).Times(0);
  EXPECT_CALL(mpx, disable).Times(0);
  EXPECT_CALL(mpx, poll_once).Times(0);
  EXPECT_CALL(mpx, handle_error).Times(0);
  EXPECT_CALL(mpx, shutdown).Times(1);

  write(*pipe_writer, detail::pollset_opcode::shutdown, nullptr,
        operation::none);
  EXPECT_EQ(updater->handle_read_event(), manager_result::done);
}

TEST_F(event_pollset_updater_test, handle_add) {
  EXPECT_CALL(mpx, add(testing::NotNull(), operation::read)).Times(1);
  EXPECT_CALL(mpx, enable).Times(0);
  EXPECT_CALL(mpx, disable).Times(0);
  EXPECT_CALL(mpx, poll_once).Times(0);
  EXPECT_CALL(mpx, handle_error).Times(0);
  EXPECT_CALL(mpx, shutdown).Times(0);

  auto [sock1, sock2] = UNPACK_EXPRESSION(net::make_stream_socket_pair());
  const net::socket_guard guard{sock1};
  auto mgr = util::make_intrusive<detail::event_handler>(sock2, &mpx);
  mgr->ref();
  EXPECT_EQ(mgr->ref_count(), 2);
  write(*pipe_writer, detail::pollset_opcode::add, mgr.get(), operation::read);
  EXPECT_EQ(updater->handle_read_event(), manager_result::ok);
  EXPECT_EQ(mgr->ref_count(), 1);
}

#if defined(LIB_NET_URING)

namespace {

class uring_multiplexer_mock : public detail::uring_multiplexer {
public:
  MOCK_METHOD(void, add, (detail::manager_base_ptr, operation), (override));

private:
  MOCK_METHOD(manager_map::iterator, del, (manager_map::iterator), (override));

public:
  MOCK_METHOD(void, enable, (detail::manager_base&, operation), (override));
  MOCK_METHOD(void, disable, (detail::manager_base&, operation, bool),
              (override));
  MOCK_METHOD(void, handle_error, (util::error), (override));
  MOCK_METHOD(void, shutdown, (), (override));
};

struct uring_pollset_updater_test : public ::testing::Test {
  uring_pollset_updater_test() {
    auto [reader, writer] = UNPACK_EXPRESSION(make_pipe());
    pipe_writer = writer;
    updater = std::make_unique<uring_pollset_updater>(reader, &mpx);
  }

  void SetUp() override {
    EXPECT_EQ(updater->init(util::config{}), util::none);
  }

  template <class... Ts>
  void write(pipe_socket handle, Ts&&... ts) {
    util::byte_buffer buf;
    util::binary_serializer sink{buf};
    sink(std::forward<Ts>(ts)...);
    EXPECT_EQ(net::write(handle, buf), static_cast<ptrdiff_t>(buf.size()));
  }

  socket_guard<pipe_socket> pipe_writer;
  uring_multiplexer_mock mpx;
  std::unique_ptr<uring_pollset_updater> updater;
};

} // namespace

TEST_F(uring_pollset_updater_test, init) {
  EXPECT_CALL(mpx, add).Times(0);
  EXPECT_CALL(mpx, enable).Times(0);
  EXPECT_CALL(mpx, disable).Times(0);
  EXPECT_CALL(mpx, handle_error).Times(0);
  EXPECT_CALL(mpx, shutdown).Times(0);
  // The updater is created and initialized in the fixture
}

TEST_F(uring_pollset_updater_test, handle_shutdown) {
  // Adding of pollset_updater and acceptor during init of mpx
  EXPECT_CALL(mpx, add).Times(2);
  EXPECT_CALL(mpx, enable).Times(0);
  EXPECT_CALL(mpx, disable).Times(1); // Disabling the pollset_updater
  EXPECT_CALL(mpx, handle_error).Times(0);
  EXPECT_CALL(mpx, shutdown).Times(1); // The actually expected operation

  EXPECT_EQ(mpx.init(detail::uring_multiplexer::manager_factory{},
                     util::config{}),
            util::none);
  mpx.set_thread_id(std::this_thread::get_id());
  EXPECT_EQ(updater->enable(operation::read), manager_result::ok);
  write(*pipe_writer, detail::pollset_opcode::shutdown, nullptr,
        operation::none);
  EXPECT_EQ(mpx.poll_once(false), util::none);
}

TEST_F(uring_pollset_updater_test, handle_add) {
  EXPECT_CALL(mpx, add(testing::NotNull(), operation::read)).Times(2);
  EXPECT_CALL(mpx, add(testing::NotNull(), operation::accept)).Times(1);
  EXPECT_CALL(mpx, enable).Times(0);
  EXPECT_CALL(mpx, disable).Times(0);
  EXPECT_CALL(mpx, handle_error).Times(0);
  EXPECT_CALL(mpx, shutdown).Times(0);

  EXPECT_EQ(mpx.init(detail::uring_multiplexer::manager_factory{},
                     util::config{}),
            util::none);
  mpx.set_thread_id(std::this_thread::get_id());

  EXPECT_EQ(updater->enable(operation::read), manager_result::ok);
  EXPECT_TRUE(updater->mask_contains(operation::read));

  auto [sock1, sock2] = UNPACK_EXPRESSION(net::make_stream_socket_pair());
  const net::socket_guard guard{sock1};
  auto mgr = util::make_intrusive<detail::event_handler>(sock2, &mpx);
  mgr->ref();
  EXPECT_EQ(mgr->ref_count(), 2);
  write(*pipe_writer, detail::pollset_opcode::add, mgr.get(), operation::read);
  mpx.set_thread_id(std::this_thread::get_id());
  EXPECT_EQ(mpx.poll_once(false), util::none);
}

#endif
