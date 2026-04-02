/**
 *  @author    Jakob Otto
 *  @file      multiplexer.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net_test.hpp"

#include "net/detail/uring_manager.hpp"
#include "net/detail/uring_multiplexer.hpp"

#include "net/ip/v4_address.hpp"
#include "net/ip/v4_endpoint.hpp"

#include "net/event_result.hpp"
#include "net/socket/stream_socket.hpp"
#include "net/socket/tcp_stream_socket.hpp"
#include "net/socket_guard.hpp"

#include "util/config.hpp"
#include "util/error.hpp"
#include "util/error_or.hpp"
#include "util/intrusive_ptr.hpp"

#include <chrono>
#include <memory>
#include <thread>
#include <tuple>

using namespace net;
using namespace net::ip;
using namespace std::chrono_literals;

namespace {

/// Shared state between actors in this test.
struct test_state {
  bool read_event_handled{false};
  bool write_event_handled{false};
  std::vector<uint64_t> handled_timeouts;
  bool register_for_writing{false};
  bool reset_timeouts{false};
};

struct dummy_socket_manager : public net::detail::uring_manager {
  dummy_socket_manager(net::socket handle, detail::multiplexer_base* mpx,
                       test_state& state)
    : detail::uring_manager(handle, mpx), state_{state} {
    // nop
  }

  /// Handle data from completed uring operations
  event_result handle_completion(operation, int) override {
    return event_result::done;
  }

  event_result handle_timeout(uint64_t timeout_id) override {
    state_.handled_timeouts.push_back(timeout_id);
    if (state_.reset_timeouts) {
      EXPECT_EQ(set_timeout_in(1ms), timeout_id + 1);
    }
    return event_result::ok;
  }

private:
  test_state& state_;
};

// -- Test fixture -------------------------------------------------------------

struct uring_multiplexer_test : public testing::Test {
  uring_multiplexer_test() {
    auto factory = [this](net::socket handle, detail::multiplexer_base* mpx) {
      return util::make_intrusive<dummy_socket_manager>(handle, mpx, state);
    };
    EXPECT_EQ(mpx.init(std::move(factory), util::config{}), util::none);
    mpx.set_thread_id(std::this_thread::get_id());
    default_num_socket_managers = mpx.num_socket_managers();
  }

  tcp_stream_socket connect_to_mpx() {
    auto sock_res = make_connected_tcp_stream_socket(
      v4_endpoint{v4_address::localhost, mpx.port()});
    EXPECT_EQ(util::get_error(sock_res), nullptr);
    auto sock = std::get<tcp_stream_socket>(sock_res);
    EXPECT_TRUE(nonblocking(sock, true));
    return sock;
  }

  bool poll_until(const std::function<bool()>& predicate, bool blocking = false,
                  const std::size_t max_num_polls = 10) {
    std::size_t num_polls = 0;
    do {
      EXPECT_EQ(mpx.poll_once(blocking), util::none);
    } while (!predicate() && (num_polls++ < max_num_polls));
    return predicate();
  }

  void enable_register_for_writing() { state.register_for_writing = true; }

  void enable_reconfigure_timeouts() { state.reset_timeouts = true; }

  bool has_handled_read_event() const { return state.read_event_handled; }

  bool has_handled_write_event() const { return state.write_event_handled; }

  detail::uring_multiplexer mpx;
  size_t default_num_socket_managers;
  test_state state;
};

bool write_all(net::tcp_stream_socket handle, util::byte_span data) {
  while (!data.empty()) {
    const auto res = write(handle, data);
    if (res > 0) {
      data = data.subspan(res);
    } else if (!net::last_socket_error_is_temporary()) {
      return false;
    }
  }
  return true;
}

bool read_all(net::tcp_stream_socket handle, util::byte_span data) {
  while (!data.empty()) {
    const auto res = read(handle, data);
    if (res > 0) {
      data = data.subspan(res);
    } else if (!net::last_socket_error_is_temporary()) {
      return false;
    }
  }
  return true;
}

} // namespace

TEST_F(uring_multiplexer_test, mpx_shuts_down_correctly) {
  EXPECT_EQ(mpx.num_socket_managers(), default_num_socket_managers);
  // Enforce a shutdown event being written to the pipe
  mpx.set_thread_id();
  mpx.shutdown();
  // No shutdown should have been initiated - All managers must still be present
  ASSERT_EQ(mpx.num_socket_managers(), default_num_socket_managers);
  // Now become the mpx thread again and poll until all mgrs are deleted
  mpx.set_thread_id(std::this_thread::get_id());
  ASSERT_TRUE(poll_until([&] { return mpx.num_socket_managers() == 0; }));
}

TEST_F(uring_multiplexer_test, mpx_accepts_connections) {
  std::array<tcp_stream_socket, 10> sockets;
  ASSERT_NE(mpx.port(), 0);
  for (auto& sock : sockets) {
    const auto num_managers = mpx.num_socket_managers();
    sock = connect_to_mpx();
    ASSERT_TRUE(poll_until(
      [&] { return mpx.num_socket_managers() == (num_managers + 1); }));
  }
  EXPECT_EQ(mpx.num_socket_managers(), default_num_socket_managers + 10);

  for (auto sock : sockets) {
    close(sock);
  }
}

TEST_F(uring_multiplexer_test, manager_removed_after_disconnect) {
  {
    socket_guard guard{connect_to_mpx()};
    ASSERT_TRUE(poll_until([&] {
      return (mpx.num_socket_managers() == (default_num_socket_managers + 1));
    }));
  }
  ASSERT_TRUE(poll_until([&] {
    return (mpx.num_socket_managers() == default_num_socket_managers);
  }));
}

TEST_F(uring_multiplexer_test, event_handling) {
  // Connect to the multiplexer and trigger it for accepting the connection
  const socket_guard guard{connect_to_mpx()};
  enable_register_for_writing();
  ASSERT_TRUE(poll_until([this] {
    return (mpx.num_socket_managers() == (default_num_socket_managers + 1));
  }));
  // Write all data and check if it will be received by the mpx
  util::byte_array<1024> buf;
  ASSERT_TRUE(write_all(guard.get(), buf));
  ASSERT_TRUE(poll_until([this] { return has_handled_read_event(); }));
  ASSERT_FALSE(has_handled_write_event());
  // Check the mgr was enabled for writing, trigger it and read from the
  ASSERT_EQ(mpx.num_socket_managers(), default_num_socket_managers + 1);
  ASSERT_TRUE(poll_until([this] { return has_handled_write_event(); }));
  ASSERT_TRUE(read_all(guard.get(), buf));
}

TEST_F(uring_multiplexer_test, resetting_timeout) {
  const std::vector<uint64_t> expected_result{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  auto res = make_stream_socket_pair();
  ASSERT_EQ(get_error(res), nullptr);
  auto sockets = std::get<stream_socket_pair>(res);
  enable_reconfigure_timeouts();
  auto mgr = util::make_intrusive<dummy_socket_manager>(sockets.first, &mpx,
                                                        state);
  mpx.add(mgr, operation::read);
  EXPECT_EQ(mgr->set_timeout_in(10ms), 0);
  ASSERT_TRUE(poll_until(
    [&] { return state.handled_timeouts.size() >= expected_result.size(); },
    true, 20));
  EXPECT_EQ(state.handled_timeouts, expected_result);
}

TEST_F(uring_multiplexer_test, multiple_timeouts) {
  const std::vector<uint64_t> expected_result{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  auto res = make_stream_socket_pair();
  ASSERT_EQ(get_error(res), nullptr);
  auto sockets = std::get<stream_socket_pair>(res);
  auto mgr = util::make_intrusive<dummy_socket_manager>(sockets.first, &mpx,
                                                        state);
  mpx.add(mgr, operation::read);
  auto duration = 10ms;
  for (size_t i = 0; i < expected_result.size(); ++i) {
    EXPECT_EQ(mgr->set_timeout_in(duration), i);
    duration += 10ms;
  }
  ASSERT_TRUE(poll_until(
    [&] { return state.handled_timeouts.size() >= expected_result.size(); },
    true, 20));
  EXPECT_EQ(state.handled_timeouts, expected_result);
}

// TODO: Implement test that checks pipe-reading and  writing for adding and
// removing socket_managers from the pollset.
