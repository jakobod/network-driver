/**
 *  @author    Jakob Otto
 *  @file      multiplexer_impl.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net_test.hpp"

#include "net/event_result.hpp"
#include "net/ip/v4_address.hpp"
#include "net/ip/v4_endpoint.hpp"
#include "net/kqueue_multiplexer.hpp"
#include "net/multiplexer.hpp"
#include "net/socket/stream_socket.hpp"
#include "net/socket/tcp_stream_socket.hpp"
#include "net/socket_guard.hpp"
#include "net/socket_manager.hpp"
#include "net/socket_manager_factory.hpp"

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

struct dummy_socket_manager : public socket_manager {
  dummy_socket_manager(net::socket handle, multiplexer* parent,
                       bool& handled_read_event, bool& handled_write_event,
                       std::vector<uint64_t>& handled_timeouts,
                       bool register_writing, bool reset_timeout)
    : socket_manager(handle, parent),
      handled_read_event_(handled_read_event),
      handled_write_event_(handled_write_event),
      handled_timeouts_(handled_timeouts),
      register_writing_(register_writing),
      reset_timeout_{reset_timeout} {
    // nop
  }

  util::error init(const util::config&) override { return util::none; }

  event_result handle_read_event() override {
    util::byte_array<1024> buf;
    handled_read_event_ = true;
    if (register_writing_) {
      this->register_writing();
    }
    return (read(handle<stream_socket>(), buf) > 0) ? event_result::ok
                                                    : event_result::error;
  }

  event_result handle_write_event() override {
    util::byte_array<1024> buf;
    handled_write_event_ = true;
    EXPECT_EQ(write(handle<stream_socket>(), buf), buf.size());
    return event_result::done;
  }

  event_result handle_timeout(uint64_t timeout_id) override {
    handled_timeouts_.push_back(timeout_id);
    if (reset_timeout_) {
      EXPECT_EQ(set_timeout_in(1ms), timeout_id + 1);
    }
    return event_result::ok;
  }

private:
  bool& handled_read_event_;
  bool& handled_write_event_;
  std::vector<uint64_t>& handled_timeouts_;
  bool register_writing_ = false;
  bool reset_timeout_ = false;
};

struct dummy_factory : public socket_manager_factory {
  dummy_factory(bool& handled_read_event, bool& handled_write_event)
    : handled_read_event_(handled_read_event),
      handled_write_event_(handled_write_event) {}

  socket_manager_ptr make(net::socket handle, multiplexer* mpx) override {
    return util::make_intrusive<dummy_socket_manager>(
      handle, mpx, handled_read_event_, handled_write_event_, handled_timeouts_,
      register_writing_, false);
  }

  void enable_register_writing() { register_writing_ = true; }

private:
  bool& handled_read_event_;
  bool& handled_write_event_;
  std::vector<uint64_t> handled_timeouts_;
  bool register_writing_ = false;
};

struct kqueue_multiplexer_test : public testing::Test {
  kqueue_multiplexer_test()
    : factory(std::make_shared<dummy_factory>(handled_read_event,
                                              handled_write_event)) {
    EXPECT_EQ(mpx.init(factory, util::config{}), util::none);
    mpx.set_thread_id(std::this_thread::get_id());
    default_num_socket_managers = mpx.num_socket_managers();
  }

  tcp_stream_socket connect_to_mpx() {
    auto sock_res = make_connected_tcp_stream_socket(
      v4_endpoint{v4_address::localhost, mpx.port()});
    EXPECT_EQ(util::get_error(sock_res), nullptr);
    return std::get<tcp_stream_socket>(sock_res);
  }

  bool poll_until(const std::function<bool()>& predicate, bool blocking = false,
                  const std::size_t max_num_polls = 10) {
    std::size_t num_polls = 0;
    do {
      EXPECT_EQ(mpx.poll_once(blocking), util::none);
    } while (!predicate() && (num_polls++ < max_num_polls));
    return predicate();
  }

  bool handled_read_event = false;
  bool handled_write_event = false;
  socket_manager_factory_ptr factory;
  kqueue_multiplexer mpx;
  size_t default_num_socket_managers;
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

TEST_F(kqueue_multiplexer_test, mpx_shuts_down_correctly) {
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

TEST_F(kqueue_multiplexer_test, mpx_accepts_connections) {
  std::array<tcp_stream_socket, 10> sockets;
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

TEST_F(kqueue_multiplexer_test, manager_removed_after_disconnect) {
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

TEST_F(kqueue_multiplexer_test, event_handling) {
  // Connect to the multiplexer and trigger it for accepting the connection
  const socket_guard guard{connect_to_mpx()};
  std::static_pointer_cast<dummy_factory>(factory)->enable_register_writing();
  ASSERT_TRUE(poll_until([this] {
    return (mpx.num_socket_managers() == (default_num_socket_managers + 1));
  }));
  // Write all data and check if it will be received by the mpx
  util::byte_array<1024> buf;
  ASSERT_TRUE(write_all(guard.get(), buf));
  ASSERT_TRUE(poll_until([this] { return handled_read_event; }));
  ASSERT_FALSE(handled_write_event);
  // Check the mgr was enabled for writing, trigger it and read from the
  ASSERT_EQ(mpx.num_socket_managers(), default_num_socket_managers + 1);
  ASSERT_TRUE(poll_until([this] { return handled_write_event; }));
  ASSERT_TRUE(read_all(guard.get(), buf));
}

TEST_F(kqueue_multiplexer_test, resetting_timeout) {
  std::vector<uint64_t> handled_timeouts;
  std::array<uint64_t, 10> expected_result{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  auto res = make_stream_socket_pair();
  ASSERT_EQ(get_error(res), nullptr);
  auto sockets = std::get<stream_socket_pair>(res);
  auto mgr = util::make_intrusive<dummy_socket_manager>(
    sockets.first, &mpx, handled_read_event, handled_write_event,
    handled_timeouts, false, true);
  mpx.add(mgr, operation::read);
  EXPECT_EQ(mgr->set_timeout_in(10ms), 0);
  ASSERT_TRUE(poll_until(
    [&] { return handled_timeouts.size() >= expected_result.size(); }, true,
    20));
  EXPECT_EQ(handled_timeouts.size(), expected_result.size());
  EXPECT_TRUE(std::equal(handled_timeouts.begin(), handled_timeouts.end(),
                         expected_result.begin()));
}

TEST_F(kqueue_multiplexer_test, multiple_timeouts) {
  std::vector<uint64_t> handled_timeouts;
  std::array<uint64_t, 10> expected_result{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  auto res = make_stream_socket_pair();
  ASSERT_EQ(get_error(res), nullptr);
  auto sockets = std::get<stream_socket_pair>(res);
  auto mgr = util::make_intrusive<dummy_socket_manager>(
    sockets.first, &mpx, handled_read_event, handled_write_event,
    handled_timeouts, false, false);
  mpx.add(mgr, operation::read);
  auto duration = 10ms;
  for (size_t i = 0; i < expected_result.size(); ++i) {
    EXPECT_EQ(mgr->set_timeout_in(duration), i);
    duration += 10ms;
  }
  ASSERT_TRUE(poll_until(
    [&] { return handled_timeouts.size() >= expected_result.size(); }, true,
    20));
  EXPECT_EQ(handled_timeouts.size(), expected_result.size());
  EXPECT_TRUE(std::equal(handled_timeouts.begin(), handled_timeouts.end(),
                         expected_result.begin()));
}

// TODO: Implement test that checks pipe-reading and  writing for adding and
// removing socket_managers from the pollset.
