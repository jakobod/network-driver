/**
 *  @author    Jakob Otto
 *  @file      stream_transport.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released
 under
 *             the GNU GPL3 License.
 */

#include "net/detail/datagram_transport.hpp"

#include "net/socket/udp_datagram_socket.hpp"

#include "net/receive_policy.hpp"
#include "net/socket_guard.hpp"

#include "net/ip/v4_address.hpp"
#include "net/ip/v4_endpoint.hpp"

#include "util/byte_literals.hpp"
#include "util/byte_span.hpp"
#include "util/config.hpp"
#include "util/error.hpp"
#include "util/error_or.hpp"

#include "multiplexer_mock.hpp"
#include "net_test.hpp"

#include <algorithm>
#include <cstring>
#include <numeric>
#include <thread>

using namespace net;
using namespace net::detail;
using namespace util::byte_literals;

namespace {

struct dummy_application {
  dummy_application(util::const_byte_span data, net::ip::v4_endpoint ep,
                    util::byte_buffer& received, uint64_t& last_timeout_id)
    : received_(received),
      data_(data),
      ep_(ep),
      last_timeout_id_(last_timeout_id) {
    // nop
  }

  util::error init(auto& parent, const util::config&) {
    parent.configure_next_read(receive_policy::up_to(16_KB));
    return util::none;
  }

  manager_result produce(auto& parent) {
    if (has_more_data()) {
      const auto size = std::min(size_t{1_KB}, data_.size());
      parent.enqueue(data_.subspan(0, size), ep_);
      data_ = data_.subspan(size);
      return manager_result::ok;
    }
    return manager_result::done;
  }

  bool has_more_data() const noexcept { return !data_.empty(); }

  manager_result consume(auto& parent, util::const_byte_span data,
                         net::ip::v4_endpoint) {
    received_.insert(received_.end(), data.begin(), data.end());
    parent.configure_next_read(receive_policy::up_to(16_KB));
    return manager_result::ok;
  }

  manager_result handle_timeout(auto&, uint64_t id) {
    last_timeout_id_ = id;
    return manager_result::ok;
  }

private:
  util::byte_buffer& received_;
  util::const_byte_span data_;
  net::ip::v4_endpoint ep_;
  uint64_t& last_timeout_id_;
};

using event_manager_type = event_datagram_transport<dummy_application>;
#if defined(LIB_NET_URING)
using uring_manager_type = uring_datagram_transport<dummy_application>;
#endif

struct datagram_transport_test : public testing::Test {
  datagram_transport_test() {
    {
      auto [socket, port] = UNPACK_EXPRESSION(make_udp_datagram_socket(0));
      reader = socket;
      reader_port = port;
    }
    {
      auto [socket, port] = UNPACK_EXPRESSION(make_udp_datagram_socket(0));
      writer = socket;
      writer_port = port;
    }
  }

  util::config cfg{};
  socket_guard<udp_datagram_socket> reader;
  std::uint16_t reader_port{0};
  socket_guard<udp_datagram_socket> writer;
  std::uint16_t writer_port{0};

  multiplexer_mock mpx;

  util::byte_buffer received_data;
  uint64_t last_timeout_id;
};

} // namespace

TEST_F(datagram_transport_test, handle_read_event) {
  static constexpr auto test_data = test::generate_test_data<16_KB>();
  net::ip::v4_endpoint receiver_ep{net::ip::v4_address::localhost, reader_port};
  event_manager_type mgr(*reader, &mpx, test_data, receiver_ep, received_data,
                         last_timeout_id);
  ASSERT_EQ(mgr.init(cfg), util::none);

  std::jthread write_thread([this] {
    const auto res = test::write_all(
      *writer, test_data,
      net::ip::v4_endpoint{net::ip::v4_address::localhost, reader_port});
    EXPECT_EQ(res, manager_result::done);
  });

  const auto invoke_result
    = test::invoke([&mgr] {
        const auto read_res = mgr.handle_read_event();
        EXPECT_NE(read_res, manager_result::done);
        EXPECT_NE(read_res, manager_result::error);
        if (read_res == manager_result::temporary_error) {
          std::this_thread::sleep_for(10ms);
        }
      }).until([this] { return received_data.size() == test_data.size(); });
  EXPECT_TRUE(invoke_result);
  EXPECT_TRUE(
    std::equal(received_data.begin(), received_data.end(), test_data.begin()));
}

TEST_F(datagram_transport_test, handle_write_event) {
  static constexpr auto test_data = test::generate_test_data<16_KB>();
  util::byte_array<16_KB> receive_buffer = {};
  net::ip::v4_endpoint receiver_ep{net::ip::v4_address::localhost, reader_port};
  event_manager_type mgr(*writer, &mpx, test_data, receiver_ep, received_data,
                         last_timeout_id);
  ASSERT_EQ(mgr.init(cfg), util::none);

  std::jthread read_thread([this, &receive_buffer] {
    const auto [res, ep] = test::read_all(*reader, receive_buffer);

    EXPECT_EQ(res, manager_result::ok);
    net::ip::v4_endpoint expected_sender{net::ip::v4_address::localhost,
                                         writer_port};
    EXPECT_EQ(ep, expected_sender);
  });

  manager_result res;
  do {
    res = mgr.handle_write_event();
    ASSERT_NE(res, manager_result::error);
    if (res == manager_result::temporary_error) {
      std::this_thread::sleep_for(10ms);
    }
  } while (res != manager_result::done);

  // Wait for the reader to completely read all data
  if (read_thread.joinable()) {
    read_thread.join();
  }

  EXPECT_EQ(receive_buffer, test_data);
}

#if defined(LIB_NET_URING)

TEST_F(datagram_transport_test, uring_handle_read_completion) {
  static constexpr auto test_data = test::generate_test_data<16_KB>();
  net::ip::v4_endpoint receiver_ep{net::ip::v4_address::localhost, reader_port};
  auto uring_mpx = UNPACK_EXPRESSION(detail::make_uring_multiplexer(
    [](net::socket, multiplexer_base*) -> uring_manager_ptr { return nullptr; },
    cfg));
  uring_manager_type mgr(*reader, uring_mpx.get(), test_data, receiver_ep,
                         received_data, last_timeout_id);
  ASSERT_EQ(mgr.init(cfg), util::none);
  mgr.enable(operation::read);

  std::atomic_bool running = true;
  std::jthread write_thread([this, &running] {
    const auto res = test::write_all(
      *writer, test_data,
      net::ip::v4_endpoint{net::ip::v4_address::localhost, reader_port});
    EXPECT_EQ(res, manager_result::done);
    running = false;
  });

  EXPECT_TRUE(test::poll_until(
    [this, &running] {
      return !running && (received_data.size() == test_data.size());
    },
    *uring_mpx, 100));
  EXPECT_EQ(received_data.size(), test_data.size());
  EXPECT_TRUE(
    std::equal(received_data.begin(), received_data.end(), test_data.begin()));
}

TEST_F(datagram_transport_test, uring_handle_write_completion) {
  static constexpr auto test_data = test::generate_test_data<16_KB>();
  util::byte_array<16_KB> receive_buffer = {};
  net::ip::v4_endpoint receiver_ep{net::ip::v4_address::localhost, reader_port};
  auto uring_mpx = UNPACK_EXPRESSION(detail::make_uring_multiplexer(
    [](net::socket, multiplexer_base*) -> uring_manager_ptr { return nullptr; },
    cfg));
  uring_manager_type mgr(*writer, uring_mpx.get(), test_data, receiver_ep,
                         received_data, last_timeout_id);
  ASSERT_EQ(mgr.init(cfg), util::none);
  mgr.enable(operation::write);

  std::atomic_bool running = true;
  std::jthread read_thread([this, &receive_buffer, &running] {
    const auto [res, ep] = test::read_all(*reader, receive_buffer);
    EXPECT_EQ(res, manager_result::ok);
    net::ip::v4_endpoint expected_sender{net::ip::v4_address::localhost,
                                         writer_port};
    EXPECT_EQ(ep, expected_sender);
    running = false;
  });

  EXPECT_TRUE(
    test::poll_until([&running] { return !running; }, *uring_mpx, 100));
  read_thread.join();
  EXPECT_EQ(receive_buffer, test_data);
}

#endif
