/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include <gtest/gtest.h>

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
  dummy_application(detail::const_byte_span data, size_t& received)
    : received_(received), data_(data) {
    // nop
  }

  template <class Parent>
  detail::error init(Parent& parent) {
    parent.configure_next_read(data_.size());
    return detail::none;
  }

  template <class Parent>
  bool produce(Parent& parent) {
    if (!written_) {
      auto& buf = parent.send_buffer();
      buf.insert(buf.end(), data_.begin(), data_.end());
      written_ = true;
      return true;
    }
    return false;
  }

  template <class Parent>
  bool consume(Parent&, detail::const_byte_span data) {
    received_ += data.size();
    EXPECT_EQ(memcmp(data.data(), data_.data(), data.size()), 0);
    return true;
  }

private:
  size_t& received_;
  bool written_ = false;
  detail::const_byte_span data_;
};

using manager_type = net::stream_manager<dummy_application>;

struct stream_manager_test : public testing::Test {
  stream_manager_test()
    : sockets{net::stream_socket{net::invalid_socket_id},
              net::stream_socket{net::invalid_socket_id}} {
    auto socket_res = net::make_stream_socket_pair();
    EXPECT_EQ(std::get_if<detail::error>(&socket_res), nullptr);
    sockets = std::get<net::stream_socket_pair>(socket_res);
    uint8_t b = 0;
    for (auto& val :
         std::span{reinterpret_cast<uint8_t*>(data.data()), data.size()})
      val = b++;
  }

  net::stream_socket_pair sockets;
  dummy_multiplexer mpx;
  detail::byte_array<32768> data;
};

} // namespace

TEST_F(stream_manager_test, handle_read_event) {
  size_t received = 0;
  manager_type mgr(sockets.first, &mpx, std::span{data}, received);
  ASSERT_EQ(mgr.init(), detail::none);
  ASSERT_EQ(write(sockets.second, data), data.size());
  mgr.handle_read_event();
  EXPECT_EQ(received, data.size());
}

TEST_F(stream_manager_test, handle_write_event) {
  // nop
}
