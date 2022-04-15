/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/epoll_multiplexer.hpp"
#include "net/ip/v4_address.hpp"
#include "net/ip/v4_endpoint.hpp"
#include "net/socket_manager_factory.hpp"
#include "net/stream_socket.hpp"
#include "net/tcp_stream_socket.hpp"

#include "util/error.hpp"

#include "net_test.hpp"

#include <chrono>

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

  util::error init() override {
    return util::none;
  }

  event_result handle_read_event() override {
    util::byte_array<1024> buf;
    handled_read_event_ = true;
    if (register_writing_)
      this->register_writing();
    return (read(handle<stream_socket>(), buf) > 0) ? event_result::ok
                                                    : event_result::error;
  }

  event_result handle_write_event() override {
    util::byte_array<1024> buf;
    handled_write_event_ = true;
    write(handle<stream_socket>(), buf);
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
      handled_write_event_(handled_write_event) {
  }

  socket_manager_ptr make(net::socket handle, multiplexer* mpx) override {
    return std::make_shared<dummy_socket_manager>(
      handle, mpx, handled_read_event_, handled_write_event_, handled_timeouts_,
      register_writing_, false);
  }

  void register_writing() {
    register_writing_ = true;
  }

private:
  bool& handled_read_event_;
  bool& handled_write_event_;
  std::vector<uint64_t> handled_timeouts_;
  bool register_writing_ = false;
};

struct epoll_multiplexer_test : public testing::Test {
  epoll_multiplexer_test()
    : factory(std::make_shared<dummy_factory>(handled_read_event,
                                              handled_write_event)) {
    mpx.set_thread_id();
    EXPECT_EQ(mpx.init(factory, 0), util::none);
    default_num_socket_managers = mpx.num_socket_managers();
  }

  tcp_stream_socket connect_to_mpx() {
    auto sock_res = make_connected_tcp_stream_socket(
      v4_endpoint{v4_address::localhost, mpx.port()});
    EXPECT_EQ(get_error(sock_res), nullptr);
    return std::get<tcp_stream_socket>(sock_res);
  }

  bool handled_read_event = false;
  bool handled_write_event = false;
  socket_manager_factory_ptr factory;
  epoll_multiplexer mpx;
  size_t default_num_socket_managers;
};

} // namespace

TEST_F(epoll_multiplexer_test, mpx_accepts_connections) {
  std::array<tcp_stream_socket, 10> sockets;
  for (auto& sock : sockets) {
    sock = connect_to_mpx();
    EXPECT_EQ(mpx.poll_once(false), util::none);
  }
  EXPECT_EQ(mpx.num_socket_managers(), default_num_socket_managers + 10);
  mpx.shutdown();
  EXPECT_EQ(mpx.num_socket_managers(), 1);
}

TEST_F(epoll_multiplexer_test, manager_removed_after_disconnect) {
  auto sock = connect_to_mpx();
  EXPECT_EQ(mpx.poll_once(false), util::none);
  EXPECT_EQ(mpx.num_socket_managers(), default_num_socket_managers + 1);
  close(sock);
  EXPECT_EQ(mpx.poll_once(false), util::none);
  EXPECT_EQ(mpx.num_socket_managers(), default_num_socket_managers);
}

TEST_F(epoll_multiplexer_test, event_handling) {
  auto sock = connect_to_mpx();
  std::static_pointer_cast<dummy_factory>(factory)->register_writing();
  EXPECT_TRUE(nonblocking(sock, true));
  EXPECT_EQ(mpx.poll_once(false), util::none);
  EXPECT_EQ(mpx.num_socket_managers(), default_num_socket_managers + 1);
  util::byte_array<1024> buf;
  write(sock, buf);
  EXPECT_EQ(mpx.poll_once(false), util::none);
  EXPECT_EQ(mpx.num_socket_managers(), default_num_socket_managers + 1);
  EXPECT_TRUE(handled_read_event);
  EXPECT_EQ(mpx.poll_once(false), util::none);
  EXPECT_TRUE(handled_write_event);
  EXPECT_EQ(read(sock, buf), buf.size());
}

TEST_F(epoll_multiplexer_test, resetting_timeout) {
  std::vector<uint64_t> handled_timeouts;
  std::array<uint64_t, 10> expected_result{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  auto res = make_stream_socket_pair();
  ASSERT_EQ(get_error(res), nullptr);
  auto sockets = std::get<stream_socket_pair>(res);
  auto mgr = std::make_shared<dummy_socket_manager>(
    sockets.first, &mpx, handled_read_event, handled_write_event,
    handled_timeouts, false, true);
  mpx.add(mgr, operation::read);
  EXPECT_EQ(mgr->set_timeout_in(10ms), 0);
  while (handled_timeouts.size() < expected_result.size())
    EXPECT_EQ(mpx.poll_once(true), util::none);
  EXPECT_EQ(handled_timeouts.size(), expected_result.size());
  EXPECT_EQ(memcmp(handled_timeouts.data(), expected_result.data(),
                   handled_timeouts.size()),
            0);
}

TEST_F(epoll_multiplexer_test, multiple_timeouts) {
  std::vector<uint64_t> handled_timeouts;
  std::array<uint64_t, 10> expected_result{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  auto res = make_stream_socket_pair();
  ASSERT_EQ(get_error(res), nullptr);
  auto sockets = std::get<stream_socket_pair>(res);
  auto mgr = std::make_shared<dummy_socket_manager>(
    sockets.first, &mpx, handled_read_event, handled_write_event,
    handled_timeouts, false, false);
  mpx.add(mgr, operation::read);
  auto duration = 10ms;
  for (size_t i = 0; i < expected_result.size(); ++i) {
    EXPECT_EQ(mgr->set_timeout_in(duration), i);
    duration += 10ms;
  }
  while (handled_timeouts.size() < expected_result.size())
    EXPECT_EQ(mpx.poll_once(true), util::none);
  EXPECT_EQ(handled_timeouts.size(), expected_result.size());
  EXPECT_EQ(memcmp(handled_timeouts.data(), expected_result.data(),
                   handled_timeouts.size()),
            0);
}
