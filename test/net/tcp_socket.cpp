/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/tcp_accept_socket.hpp"
#include "net/tcp_stream_socket.hpp"

#include "util/error.hpp"

#include "net_test.hpp"

using namespace net;

TEST(tcp_socket_test, accept) {
  auto acc_res = make_tcp_accept_socket(0);
  ASSERT_EQ(get_error(acc_res), nullptr);
  auto acc_pair = std::get<acceptor_pair>(acc_res);
  // Connect to the accept socket
  auto conn_res = make_connected_tcp_stream_socket("127.0.0.1",
                                                   acc_pair.second);
  ASSERT_EQ(get_error(conn_res), nullptr);
  auto sock = std::get<tcp_stream_socket>(conn_res);
  ASSERT_NE(sock, invalid_socket);
  // Accept the connection
  auto accepted = accept(acc_pair.first);
  EXPECT_NE(accepted, invalid_socket);
  // Check functionality
  util::byte_array<10> data;
  EXPECT_EQ(write(sock, data), data.size());
  EXPECT_EQ(read(accepted, data), data.size());
}
