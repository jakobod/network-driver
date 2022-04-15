/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/stream_transport.hpp"
#include "net/transport.hpp"

#include "net/multiplexer.hpp"
#include "net/receive_policy.hpp"
#include "net/stream_socket.hpp"

#include "util/error.hpp"

#include "net_test.hpp"

#include <algorithm>
#include <cstring>
#include <numeric>

using namespace net;

namespace {

struct dummy_multiplexer : public multiplexer {
  util::error init(socket_manager_factory_ptr, uint16_t) override {
    return util::none;
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

  bool running() const override {
    return false;
  }

  void handle_error(const util::error& err) override {
    FAIL() << "There should be no errors! " << err << std::endl;
  }

  util::error poll_once(bool) override {
    return util::none;
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

  uint64_t set_timeout(socket_manager&,
                       std::chrono::system_clock::time_point) override {
    return 0;
  }
};

struct dummy_application {
  dummy_application(transport& parent, util::const_byte_span data,
                    util::byte_buffer& received)
    : received_(received), data_(data), parent_(parent) {
    // nop
  }

  util::error init() {
    parent_.configure_next_read(receive_policy::exactly(1024));
    return util::none;
  }

  event_result produce() {
    auto size = std::min(size_t{1024}, data_.size());
    parent_.enqueue({data_.begin(), size});
    data_ = data_.subspan(size);
    return event_result::ok;
  }

  bool has_more_data() {
    return !data_.empty();
  }

  event_result consume(util::const_byte_span data) {
    received_.insert(received_.end(), data.begin(), data.end());
    parent_.configure_next_read(receive_policy::exactly(1024));
    return event_result::ok;
  }

  event_result handle_timeout(uint64_t) {
    return event_result::ok;
  }

private:
  util::byte_buffer& received_;
  util::const_byte_span data_;

  transport& parent_;
};

using manager_type = stream_transport<dummy_application>;

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
  }

  stream_socket_pair sockets;
  dummy_multiplexer mpx;
  util::byte_array<32768> data;

  util::byte_buffer received_data;
};

} // namespace

TEST_F(stream_manager_test, handle_read_event) {
  manager_type mgr(sockets.first, &mpx, std::span{data}, received_data);
  ASSERT_EQ(mgr.init(), util::none);
  ASSERT_EQ(write(sockets.second, data), data.size());
  while (received_data.size() < data.size())
    ASSERT_EQ(mgr.handle_read_event(), event_result::ok);
  EXPECT_TRUE(
    std::equal(received_data.begin(), received_data.end(), data.begin()));
}

TEST_F(stream_manager_test, handle_write_event) {
  size_t received = 0;
  util::byte_array<32768> buf;
  manager_type mgr(sockets.first, &mpx, std::span{data}, received_data);
  ASSERT_EQ(mgr.init(), util::none);
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
  while (mgr.handle_write_event() == event_result::ok)
    read_some();
  read_some();
  ASSERT_EQ(received, data.size());
  EXPECT_EQ(memcmp(data.data(), received_data.data(), received_data.size()), 0);
}

TEST_F(stream_manager_test, disconnect) {
  manager_type mgr(sockets.first, &mpx, std::span{data}, received_data);
  ASSERT_EQ(mgr.init(), util::none);
  close(sockets.second);
  EXPECT_EQ(mgr.handle_read_event(), event_result::error);
}
