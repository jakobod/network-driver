/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "fwd.hpp"

#include "net/datagram_socket.hpp"

#include "util/error.hpp"

#include <gtest/gtest.h>

#include <algorithm>

using namespace net;

TEST(datagram_socket, write) {
  // Create socketpair
  auto maybe_sockets = make_connected_datagram_socket_pair();
  ASSERT_EQ(nullptr, get_error(maybe_sockets));
  auto sockets = std::get<datagram_socket_pair>(maybe_sockets);
  // create and send the testdata
  util::byte_array<1024> data;
  std::uint8_t i = 0;
  for (auto& v : data)
    v = std::byte{i++};
  ASSERT_EQ(data.size(), write(sockets.first, data));
  // receive and check the testdata
  util::byte_array<1024> read_buffer;
  ASSERT_EQ(read_buffer.size(), read(sockets.second, read_buffer));
  ASSERT_TRUE(std::equal(read_buffer.begin(), read_buffer.end(), data.begin()));
}
