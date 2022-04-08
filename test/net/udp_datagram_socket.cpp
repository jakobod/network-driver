/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "fwd.hpp"

#include "net/socket_guard.hpp"
#include "net/udp_datagram_socket.hpp"

#include "net/ip/v4_address.hpp"
#include "net/ip/v4_endpoint.hpp"

#include "util/error.hpp"

#include "net_test.hpp"

#include <algorithm>

using namespace net;
using namespace net::ip;

namespace {

constexpr const v4_address localhost{util::make_byte_array(127, 0, 0, 1)};

struct udp_datagram_socket_test : public testing::Test {
  udp_datagram_socket_test() {
    std::uint8_t i = 0;
    for (auto& v : data)
      v = std::byte{i++};
  }

  util::byte_array<1024> data;
};

} // namespace

TEST_F(udp_datagram_socket_test, write) {
  // Create send socket
  auto maybe_recv_socket_res = make_udp_datagram_socket(0);
  ASSERT_EQ(nullptr, get_error(maybe_recv_socket_res));
  auto [send_sock, send_port]
    = std::get<udp_datagram_socket_result>(maybe_recv_socket_res);
  auto send_guard = net::make_socket_guard(send_sock);
  ASSERT_TRUE(nonblocking(send_sock, true));
  v4_endpoint src_ep{localhost, send_port};
  // Create send socket
  auto maybe_send_socket_res = make_udp_datagram_socket(0);
  ASSERT_EQ(nullptr, get_error(maybe_send_socket_res));
  auto [recv_sock, recv_port]
    = std::get<udp_datagram_socket_result>(maybe_send_socket_res);
  auto recv_guard = net::make_socket_guard(recv_sock);
  ASSERT_TRUE(nonblocking(recv_sock, true));
  v4_endpoint dst_ep{localhost, recv_port};
  // Write the testdata
  ASSERT_EQ(data.size(), write(send_sock, dst_ep, data));
  // receive and check the testdata
  util::byte_array<1024> recv_buffer;
  auto [ep, res] = read(recv_sock, recv_buffer);
  ASSERT_EQ(recv_buffer.size(), res);
  ASSERT_TRUE(std::equal(recv_buffer.begin(), recv_buffer.end(), data.begin()));
}
