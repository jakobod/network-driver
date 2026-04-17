/**
 *  @author    Jakob Otto
 *  @file      stream_transport.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released
 under
 *             the GNU GPL3 License.
 */

#include "net/detail/stream_transport.hpp"

#include "net/detail/event_handler.hpp"
#if defined(LIB_NET_URING)
#  include "net/detail/uring_manager.hpp"
#endif

#include "net/receive_policy.hpp"
#include "net/socket/stream_socket.hpp"

#include "util/byte_span.hpp"
#include "util/config.hpp"
#include "util/error.hpp"
#include "util/error_or.hpp"

#include "multiplexer_mock.hpp"
#include "net_test.hpp"

#include <algorithm>
#include <cstring>
#include <numeric>
#include <ranges>
#include <thread>

using namespace net;

namespace {

struct test_data {
  util::byte_buffer received;
  util::const_byte_span data;
  uint64_t last_timeout_id;
};

struct dummy_application {
  static constexpr std::size_t min_read_size = 1024;

  dummy_application(test_data& data) : data_(data) {
    // nop
  }

  util::error init(auto& parent, const util::config&) {
    parent.configure_next_read(receive_policy::exactly(1024));
    return util::none;
  }

  manager_result produce(auto& parent) {
    const auto size = std::min(size_t{1024}, data_.data.size());
    parent.enqueue(data_.data.first(size));
    data_.data = data_.data.subspan(size);
    return manager_result::ok;
  }

  bool has_more_data() const noexcept { return !data_.data.empty(); }

  manager_result consume(auto& parent, util::const_byte_span data) {
    data_.received.insert(data_.received.end(), data.begin(), data.end());
    parent.configure_next_read(receive_policy::exactly(min_read_size));
    return manager_result::ok;
  }

  manager_result handle_timeout(auto&, uint64_t id) {
    data_.last_timeout_id = id;
    return manager_result::ok;
  }

private:
  test_data& data_;
};

using event_stream_transport
  = detail::stream_transport<detail::event_handler, dummy_application>;

struct event_stream_transport_test : public testing::Test, public test_data {
  event_stream_transport_test()
    : sockets{UNPACK_EXPRESSION(make_stream_socket_pair())},
      mgr{sockets.first, &mpx, *this} {
    for (size_t i = 0; i < data_buffer.size(); ++i) {
      data_buffer[i] = static_cast<std::byte>(i & 0xFF);
    }
    data = data_buffer;
  }

  ~event_stream_transport_test() {
    close(sockets.first);
    close(sockets.second);
  }

  void SetUp() override {
    ASSERT_TRUE(nonblocking(sockets.second, true));
    ASSERT_EQ(mgr.init(cfg), util::none);
  }

  util::byte_array<32768> data_buffer;
  util::config cfg;
  stream_socket_pair sockets;
  multiplexer_mock mpx;
  event_stream_transport mgr;
};

} // namespace

TEST_F(event_stream_transport_test, handle_read_event) {
  std::thread writer{[this] { test::write_all(sockets.second, data); }};
  while (received.size() < data.size()) {
    const auto read_res = mgr.handle_read_event();
    ASSERT_NE(read_res, manager_result::done);
    ASSERT_NE(read_res, manager_result::error);
  }
  if (writer.joinable()) {
    writer.join();
  }
  EXPECT_TRUE(
    std::equal(received.begin(), received.end(), data_buffer.begin()));
}

TEST_F(event_stream_transport_test, partial_read) {
  while (received.size() < data.size()) {
    const auto ev_res = test::write_all(
      sockets.second,
      data.subspan(0, std::min(data.size(),
                               (dummy_application::min_read_size / 2))));
    EXPECT_EQ(ev_res, manager_result::done);
    data = data.subspan(dummy_application::min_read_size / 2);
    const auto read_res = mgr.handle_read_event();
    ASSERT_NE(read_res, manager_result::done);
    ASSERT_NE(read_res, manager_result::error);
  }
  EXPECT_TRUE(
    std::equal(received.begin(), received.end(), data_buffer.begin()));
}

TEST_F(event_stream_transport_test, handle_write_event) {
  size_t received = 0;
  util::byte_array<32768> buf;

  while (received < buf.size()) {
    while (mgr.handle_write_event() == manager_result::ok)
      ;
    const auto read_res = read(
      sockets.second, std::span{buf.data() + received, buf.size() - received});
    ASSERT_GT(read_res, 0);
    received += read_res;
  }
  ASSERT_EQ(received, data_buffer.size());
  EXPECT_EQ(memcmp(data_buffer.data(), this->received.data(),
                   this->received.size()),
            0);
}

TEST_F(event_stream_transport_test, disconnect) {
  close(this->sockets.second);
  EXPECT_EQ(mgr.handle_read_event(), manager_result::done);
}

TEST_F(event_stream_transport_test, timeout_handling) {
  EXPECT_EQ(mgr.handle_timeout(42), manager_result::ok);
  EXPECT_EQ(last_timeout_id, 42);
}

#if defined(LIB_NET_URING)

using uring_stream_transport
  = detail::stream_transport<detail::uring_manager, dummy_application>;

struct uring_stream_transport_test : public testing::Test, public test_data {
  uring_stream_transport_test()
    : sockets{UNPACK_EXPRESSION(make_stream_socket_pair())},
      mgr{sockets.first, &mpx, *this} {
    for (size_t i = 0; i < data_buffer.size(); ++i) {
      data_buffer[i] = static_cast<std::byte>(i & 0xFF);
    }
    data = data_buffer;
  }

  ~uring_stream_transport_test() {
    close(sockets.first);
    close(sockets.second);
  }

  void SetUp() override { ASSERT_EQ(mgr.init(cfg), util::none); }

  util::byte_array<32768> data_buffer;
  util::config cfg;
  stream_socket_pair sockets;
  multiplexer_mock mpx;
  uring_stream_transport mgr;
};

TEST_F(uring_stream_transport_test, handles_received_data) {
  // Continuously fill the buffer of the uring_manager with the desired number
  // of bytes and trigger handling
  while (!data.empty()) {
    auto read_buffer = mgr.read_buffer();
    const auto* ptr = data.data();
    const auto size = std::min(data.size(), read_buffer.size());
    std::memcpy(read_buffer.data(), ptr, size);
    data = data.subspan(size);
    ASSERT_EQ(mgr.handle_completion(operation::read, static_cast<int>(size), 0),
              manager_result::ok);
  }
  // After writing all data, the application should have consumed all data
  EXPECT_TRUE(
    std::equal(received.begin(), received.end(), data_buffer.begin()));
}

TEST_F(uring_stream_transport_test, partial_reads) {
  // Continuously fill the buffer of the uring_manager with the desired number
  // of bytes and trigger handling
  while (!data.empty()) {
    auto read_buffer = mgr.read_buffer();
    const auto* ptr = data.data();
    const auto size = std::min(data.size(),
                               (dummy_application::min_read_size / 2));
    std::memcpy(read_buffer.data(), ptr, size);
    data = data.subspan(size);
    ASSERT_EQ(mgr.handle_completion(operation::read, static_cast<int>(size), 0),
              manager_result::ok);
  }
  // After writing all data, the application should have consumed all data
  EXPECT_TRUE(
    std::equal(received.begin(), received.end(), data_buffer.begin()));
}

TEST_F(uring_stream_transport_test, prepares_data_to_write) {
  static constexpr std::size_t max_retries = 20;
  size_t received = 0;
  util::byte_buffer buf;

  for (std::size_t i = 0; i < max_retries; ++i) {
    auto& write_queue = mgr.write_queue();
    const auto num_bytes_this_try = std::accumulate(
      write_queue.begin(), write_queue.end(), std::size_t{0},
      [](std::size_t sum, const auto& buf) { return sum + buf.size(); });
    received += num_bytes_this_try;
    const auto res = mgr.handle_completion(operation::write, num_bytes_this_try,
                                           0);
    ASSERT_NE(res, manager_result::error);
    ASSERT_NE(res, manager_result::temporary_error);
    if (res == manager_result::done) {
      break;
    }
  }
  EXPECT_EQ(received, data_buffer.size());
  EXPECT_TRUE(std::equal(buf.begin(), buf.end(), data_buffer.begin()));
}

TEST_F(uring_stream_transport_test, disconnect) {
  EXPECT_EQ(mgr.handle_completion(operation::read, 0, 0), manager_result::done);
}

TEST_F(uring_stream_transport_test, timeout_handling) {
  ASSERT_EQ(mgr.handle_timeout(42), manager_result::ok);
  EXPECT_EQ(last_timeout_id, 42);
}

#endif
