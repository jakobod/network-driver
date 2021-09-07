/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include <gtest/gtest.h>

#include "net/tcp_accept_socket.hpp"
#include "net/tcp_stream_socket.hpp"

TEST(tcp_accept_socket_test, accept) {
  auto acc_res = net::make_tcp_accept_socket(0);
  ASSERT_EQ(std::get_if<detail::error>(&acc_res), nullptr);
  auto acc_pair = std::get<net::acceptor_pair>(acc_res);
  // Connect to the accept socket
  auto conn_res = net::make_connected_tcp_stream_socket("127.0.0.1",
                                                        acc_pair.second);
  ASSERT_EQ(std::get_if<detail::error>(&conn_res), nullptr);
  auto sock = std::get<net::tcp_stream_socket>(conn_res);
  ASSERT_NE(sock, net::invalid_socket);
  // Accept the connection
  auto accepted = accept(acc_pair.first);
  EXPECT_NE(accepted, net::invalid_socket);
}
