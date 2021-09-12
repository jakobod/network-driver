/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <numeric>

#include "detail/error.hpp"
#include "fwd.hpp"
#include "net/multiplexer.hpp"
#include "net/stream_manager.hpp"
#include "net/stream_socket.hpp"

namespace {

struct dummy_multiplexer : public net::multiplexer {
  detail::error init(net::socket_manager_factory_ptr) override {
    return detail::none;
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

  void handle_error(detail::error err) override {
    FAIL() << "There should be no errors! " << err << std::endl;
  }

  void poll_once(bool) override {
    // nop
  }

  void add(net::socket_manager_ptr, net::operation) override {
    // nop
  }

  void enable(net::socket_manager&, net::operation) override {
    // nop
  }

  void disable(net::socket_manager&, net::operation, bool) override {
    // nop
  }
};

struct dummy_application {
  dummy_application(detail::const_byte_span data, detail::byte_buffer& received)
    : received_(received), data_(data) {
    // nop
  }

  template <class Parent>
  detail::error init(Parent& parent) {
    parent.configure_next_read(1024);
    return detail::none;
  }

  template <class Parent>
  void produce(Parent& parent) {
    auto& buf = parent.send_buffer();
    auto size = std::min(size_t{1024}, data_.size());
    buf.insert(buf.end(), data_.begin(), data_.begin() + size);
    data_ = data_.subspan(size);
  }

  bool has_more_data() {
    return !data_.empty();
  }

  template <class Parent>
  bool consume(Parent& parent, detail::const_byte_span data) {
    received_.insert(received_.end(), data.begin(), data.end());
    parent.configure_next_read(1024);
    return true;
  }

private:
  detail::byte_buffer& received_;
  detail::const_byte_span data_;
};

using manager_type = net::stream_manager<dummy_application>;

struct stream_manager_test : public testing::Test {
  stream_manager_test()
    : sockets{net::stream_socket{net::invalid_socket_id},
              net::stream_socket{net::invalid_socket_id}} {
    auto socket_res = net::make_stream_socket_pair();
    EXPECT_EQ(detail::get_error(socket_res), nullptr);
    sockets = std::get<net::stream_socket_pair>(socket_res);
    uint8_t b = 0;
    for (auto& val :
         std::span{reinterpret_cast<uint8_t*>(data.data()), data.size()})
      val = b++;
    EXPECT_TRUE(net::nonblocking(sockets.second, true));
  }

  net::stream_socket_pair sockets;
  dummy_multiplexer mpx;
  detail::byte_array<32768> data;

  detail::byte_buffer received_data;
};

} // namespace

TEST_F(stream_manager_test, handle_read_event) {
  manager_type mgr(sockets.first, &mpx, std::span{data}, received_data);
  ASSERT_EQ(mgr.init(), detail::none);
  ASSERT_EQ(net::write(sockets.second, data), data.size());
  while (received_data.size() < data.size())
    ASSERT_TRUE(mgr.handle_read_event());
  EXPECT_TRUE(
    std::equal(received_data.begin(), received_data.end(), data.begin()));
}

TEST_F(stream_manager_test, disconnect) {
  manager_type mgr(sockets.first, &mpx, std::span{data}, received_data);
  ASSERT_EQ(mgr.init(), detail::none);
  close(sockets.second);
  EXPECT_FALSE(mgr.handle_read_event());
}

TEST_F(stream_manager_test, handle_write_event) {
  size_t received = 0;
  detail::byte_array<32768> buf;
  manager_type mgr(sockets.first, &mpx, std::span{data}, received_data);
  ASSERT_EQ(mgr.init(), detail::none);
  auto read_some = [&]() {
    auto data = buf.data() + received;
    auto remaining = buf.size() - received;
    auto res = net::read(sockets.second, std::span{data, remaining});
    if (res < 0)
      ASSERT_TRUE(net::last_socket_error_is_temporary());
    else if (res == 0)
      FAIL() << "socket diconnected prematurely!" << std::endl;
    received += res;
  };
  while (mgr.handle_write_event())
    read_some();
  read_some();
  ASSERT_EQ(received, data.size());
  EXPECT_EQ(memcmp(data.data(), received_data.data(), received_data.size()), 0);
}
