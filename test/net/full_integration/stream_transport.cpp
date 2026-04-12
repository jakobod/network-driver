/**
 *  @author    Jakob Otto
 *  @file      multiplexer.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net_test.hpp"

#include "net/detail/manager_base.hpp"
#include "net/detail/stream_transport.hpp"
#if defined(LIB_NET_URING)
#  include "net/detail/uring_manager.hpp"
#  include "net/detail/uring_multiplexer.hpp"
#endif

#include "net/multiplexer.hpp"

#include "net/ip/v4_address.hpp"
#include "net/ip/v4_endpoint.hpp"

#include "net/manager_result.hpp"
#include "net/socket/stream_socket.hpp"
#include "net/socket/tcp_stream_socket.hpp"
#include "net/socket_guard.hpp"

#include "util/byte_literals.hpp"
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
using namespace util::byte_literals;
using namespace std::chrono_literals;

namespace {

struct mirror_application {
  static constexpr std::size_t min_read_size = 1_KB;
  static constexpr std::size_t max_read_size = 8_KB;

  mirror_application(detail::transport_base& parent) : parent_{parent} {
    // nop
  }

  util::error init(const util::config&) {
    parent_.configure_next_read(
      receive_policy::between(min_read_size, max_read_size));
    return util::none;
  }

  manager_result produce() { return manager_result::ok; }

  bool has_more_data() const noexcept { return false; }

  manager_result consume(util::const_byte_span data) {
    parent_.enqueue(data);
    parent_.configure_next_read(
      receive_policy::between(min_read_size, max_read_size));
    return manager_result::ok;
  }

  manager_result handle_timeout(uint64_t) { return manager_result::ok; }

private:
  detail::transport_base& parent_;
};

// -- Test fixture -------------------------------------------------------------

template <typename FactoryCreator>
struct stream_transport_full_integration : public testing::Test {
  stream_transport_full_integration() {
    for (std::size_t i = 0; i < data.size(); ++i) {
      data[i] = static_cast<std::byte>(i);
    }
    FactoryCreator::create_multiplexer(cfg, mpx, default_num_socket_managers);
  }

  void SetUp() override {
    mpx->start();
    socket = connect_to_mpx();
    const auto res = test::wait_for([this] {
      return mpx->num_socket_managers() >= (default_num_socket_managers + 1);
    });
    ASSERT_TRUE(res);
    ASSERT_TRUE(nonblocking(*socket, true));
  }

  void TearDown() override {
    mpx->shutdown();
    mpx->join();
  }

  tcp_stream_socket connect_to_mpx() {
    auto sock = UNPACK_EXPRESSION(make_connected_tcp_stream_socket(
      v4_endpoint{v4_address::localhost, mpx->port()}));
    return sock;
  }

  detail::multiplexer_base_ptr mpx;
  net::socket_guard<tcp_stream_socket> socket;
  std::size_t default_num_socket_managers{0};
  util::config cfg;
  util::byte_array<10_KB> data;
};

} // namespace

// -- Factory creators --------------------------------------------------------

struct event_based {
  static void create_multiplexer(util::config& cfg,
                                 detail::multiplexer_base_ptr& mpx,
                                 std::size_t& num_managers) {
    auto factory
      = [](net::socket handle,
           detail::multiplexer_base* mpx) -> detail::event_handler_ptr {
      return util::make_intrusive<
        detail::event_stream_transport<mirror_application>>(
        socket_cast<stream_socket>(handle), mpx);
    };
    mpx = UNPACK_EXPRESSION(net::make_multiplexer(std::move(factory), cfg));
    num_managers = mpx->num_socket_managers();
  }
};

#if defined(LIB_NET_URING)

struct uring_based {
  static void create_multiplexer(util::config& cfg,
                                 detail::multiplexer_base_ptr& mpx,
                                 std::size_t& num_managers) {
    auto factory
      = [](net::socket handle,
           detail::uring_multiplexer* mpx) -> detail::uring_manager_ptr {
      return util::make_intrusive<
        detail::uring_stream_transport<mirror_application>>(
        socket_cast<stream_socket>(handle), mpx);
    };
    mpx = UNPACK_EXPRESSION(
      detail::make_uring_multiplexer(std::move(factory), cfg));
    num_managers = mpx->num_socket_managers();
  }
};

#endif

// -- Parameterized fixture ---------------------------------------------------

using FactoryCreators = ::testing::Types<event_based
#if defined(LIB_NET_URING)
                                         ,
                                         uring_based
#endif
                                         >;

TYPED_TEST_SUITE(stream_transport_full_integration, FactoryCreators);

TYPED_TEST(stream_transport_full_integration, mirror) {
  // Send 10kB
  {
    std::size_t written = 0;
    for (std::size_t i = 0; (i < 10) && written < this->data.size(); ++i) {
      const auto [event_res, num_bytes_written]
        = test::write(*this->socket, std::span{(this->data.data() + written),
                                               (this->data.size() - written)});
      ASSERT_NE(event_res, manager_result::error);
      if (event_res == manager_result::temporary_error) {
        std::this_thread::sleep_for(10ms);
      }
      written += num_bytes_written;
    }
    ASSERT_EQ(written, this->data.size());
  }
  // receive it all
  util::byte_array<10_KB> receive_buffer = {};
  {
    std::size_t received = 0;
    for (std::size_t i = 0; (i < 10) && (received < 10_KB); ++i) {
      const auto [event_res, num_bytes_received] = test::read(
        *this->socket, std::span{(receive_buffer.data() + received),
                                 (receive_buffer.size() - received)});
      ASSERT_NE(event_res, manager_result::error);
      if (event_res == manager_result::temporary_error) {
        std::this_thread::sleep_for(10ms);
      }
      received += num_bytes_received;
    }
    ASSERT_EQ(received, 10_KB);
  }
  EXPECT_EQ(this->data, receive_buffer);
}
