/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <numeric>

#include "fwd.hpp"
#include "net/error.hpp"
#include "net/multiplexer.hpp"
#include "net/stream_manager.hpp"
#include "net/stream_socket.hpp"

using namespace net;

namespace {

struct dummy_multiplexer : public multiplexer {
  error init(socket_manager_factory_ptr, uint16_t) override {
    return none;
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

  bool running() override {
    return false;
  }

  void handle_error(error err) override {
    FAIL() << "There should be no errors! " << err << std::endl;
  }

  error poll_once(bool) override {
    return none;
  }

  void add(socket_manager_ptr, operation) override {
    // nop
  }

  void enable(socket_manager&, operation) override {
    // nop
  }

  void disable(socket_manager&, operation, bool) override {
    // nop
  }

  void set_timeout(socket_manager*, uint64_t,
                   std::chrono::system_clock::time_point) override {
    // nop
  }
};

struct dummy_application {
  dummy_application(util::const_byte_span data, util::byte_buffer& received)
    : received_(received), data_(data) {
    // nop
  }

  template <class Parent>
  error init(Parent& parent) {
    parent.configure_next_read(1024);
    return none;
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
  bool consume(Parent& parent, util::const_byte_span data) {
    received_.insert(received_.end(), data.begin(), data.end());
    parent.configure_next_read(1024);
    return true;
  }

private:
  util::byte_buffer& received_;
  util::const_byte_span data_;
};

using manager_type = stream_manager<dummy_application>;

struct stream_manager_test : public testing::Test {
  stream_manager_test()
    : sockets{stream_socket{invalid_socket_id},
              stream_socket{invalid_socket_id}} {
    auto socket_res = make_stream_socket_pair();
    EXPECT_EQ(get_error(socket_res), nullptr);
    sockets = std::get<stream_socket_pair>(socket_res);
    uint8_t b = 0;
    for (auto& val :
         std::span{reinterpret_cast<uint8_t*>(data.data()), data.size()})
      val = b++;
    EXPECT_TRUE(nonblocking(sockets.second, true));
  }

  stream_socket_pair sockets;
  dummy_multiplexer mpx;
  util::byte_array<32768> data;

  util::byte_buffer received_data;
};

} // namespace

TEST_F(stream_manager_test, handle_read_event) {
  manager_type mgr(sockets.first, &mpx, std::span{data}, received_data);
  ASSERT_EQ(mgr.init(), none);
  ASSERT_EQ(write(sockets.second, data), data.size());
  while (received_data.size() < data.size())
    ASSERT_TRUE(mgr.handle_read_event());
  EXPECT_TRUE(
    std::equal(received_data.begin(), received_data.end(), data.begin()));
}

TEST_F(stream_manager_test, disconnect) {
  manager_type mgr(sockets.first, &mpx, std::span{data}, received_data);
  ASSERT_EQ(mgr.init(), none);
  close(sockets.second);
  EXPECT_FALSE(mgr.handle_read_event());
}

TEST_F(stream_manager_test, handle_write_event) {
  size_t received = 0;
  util::byte_array<32768> buf;
  manager_type mgr(sockets.first, &mpx, std::span{data}, received_data);
  ASSERT_EQ(mgr.init(), none);
  auto read_some = [&]() {
    auto data = buf.data() + received;
    auto remaining = buf.size() - received;
    auto res = read(sockets.second, std::span{data, remaining});
    if (res < 0)
      ASSERT_TRUE(last_socket_error_is_temporary());
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
