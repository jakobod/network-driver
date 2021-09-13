/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include <gtest/gtest.h>

#include "fwd.hpp"
#include "net/epoll_multiplexer.hpp"
#include "net/error.hpp"
#include "net/socket_manager_factory.hpp"
#include "net/stream_socket.hpp"
#include "net/tcp_stream_socket.hpp"

using namespace net;

namespace {

struct dummy_socket_manager : public socket_manager {
  dummy_socket_manager(net::socket handle, multiplexer* parent,
                       bool& handled_read_event, bool& handled_write_event,
                       bool register_writing)
    : socket_manager(handle, parent),
      handled_read_event_(handled_read_event),
      handled_write_event_(handled_write_event),
      register_writing_(register_writing) {
    // nop
  }

  bool handle_read_event() override {
    detail::byte_array<1024> buf;
    handled_read_event_ = true;
    if (register_writing_)
      this->register_writing();
    return read(handle<stream_socket>(), buf) > 0;
  }

  bool handle_write_event() override {
    detail::byte_array<1024> buf;
    handled_write_event_ = true;
    write(handle<stream_socket>(), buf);
    return false;
  }

private:
  bool& handled_read_event_;
  bool& handled_write_event_;
  bool register_writing_ = false;
};

struct dummy_factory : public socket_manager_factory {
  dummy_factory(bool& handled_read_event, bool& handled_write_event)
    : handled_read_event_(handled_read_event),
      handled_write_event_(handled_write_event) {
  }

  socket_manager_ptr make(net::socket handle, multiplexer* mpx) override {
    return std::make_shared<dummy_socket_manager>(handle, mpx,
                                                  handled_read_event_,
                                                  handled_write_event_,
                                                  register_writing_);
  }

  void register_writing() {
    register_writing_ = true;
  }

private:
  bool& handled_read_event_;
  bool& handled_write_event_;
  bool register_writing_ = false;
};

struct epoll_multiplexer_test : public testing::Test {
  epoll_multiplexer_test()
    : factory(std::make_shared<dummy_factory>(handled_read_event,
                                              handled_write_event)) {
    mpx.set_thread_id();
    EXPECT_EQ(mpx.init(factory), none);
    default_num_socket_managers = mpx.num_socket_managers();
  }

  tcp_stream_socket connect_to_mpx() {
    auto sock_res = make_connected_tcp_stream_socket("127.0.0.1", mpx.port());
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
  for (size_t i = 0; i < 10; ++i) {
    connect_to_mpx();
    EXPECT_EQ(mpx.poll_once(false), none);
  }
  EXPECT_EQ(mpx.num_socket_managers(), default_num_socket_managers + 10);
  mpx.shutdown();
  EXPECT_EQ(mpx.num_socket_managers(), 1);
}

TEST_F(epoll_multiplexer_test, manager_removed_after_disconnect) {
  auto sock = connect_to_mpx();
  EXPECT_EQ(mpx.poll_once(false), none);
  EXPECT_EQ(mpx.num_socket_managers(), default_num_socket_managers + 1);
  close(sock);
  EXPECT_EQ(mpx.poll_once(false), none);
  EXPECT_EQ(mpx.num_socket_managers(), default_num_socket_managers);
}

TEST_F(epoll_multiplexer_test, event_handling) {
  auto sock = connect_to_mpx();
  std::static_pointer_cast<dummy_factory>(factory)->register_writing();
  EXPECT_TRUE(nonblocking(sock, true));
  EXPECT_EQ(mpx.poll_once(false), none);
  EXPECT_EQ(mpx.num_socket_managers(), default_num_socket_managers + 1);
  detail::byte_array<1024> buf;
  write(sock, buf);
  EXPECT_EQ(mpx.poll_once(false), none);
  EXPECT_EQ(mpx.num_socket_managers(), default_num_socket_managers + 1);
  EXPECT_TRUE(handled_read_event);
  EXPECT_EQ(mpx.poll_once(false), none);
  EXPECT_TRUE(handled_write_event);
  EXPECT_EQ(read(sock, buf), buf.size());
}