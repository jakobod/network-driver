/**
 *  @author    Jakob Otto
 *  @file      socket_manager.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/detail/manager_base.hpp"

#include "net/event_result.hpp"
#include "net/socket/stream_socket.hpp"

#include "util/byte_buffer.hpp"
#include "util/config.hpp"
#include "util/error.hpp"
#include "util/error_or.hpp"

#include "multiplexer_mock.hpp"
#include "net_test.hpp"

#include <algorithm>
#include <cstring>
#include <numeric>

using namespace net;

namespace {

struct manager_base_test : public testing::Test {
  manager_base_test()
    : sockets{stream_socket{invalid_socket_id},
              stream_socket{invalid_socket_id}} {
    sockets = UNPACK_EXPRESSION(net::make_stream_socket_pair());
    EXPECT_TRUE(nonblocking(sockets.first, true));
    EXPECT_TRUE(nonblocking(sockets.second, true));
  }

  stream_socket_pair sockets;
  multiplexer_mock mpx;
};

} // namespace

TEST_F(manager_base_test, construction_and_destruction) {
  {
    const detail::manager_base mgr{sockets.first, &mpx};
    EXPECT_EQ(mgr.mpx(), &mpx);
    EXPECT_EQ(mgr.handle(), sockets.first);
    EXPECT_EQ(mgr.mask(), operation::none);
  }
  util::byte_buffer buf(1);
  EXPECT_EQ(read(sockets.second, buf), 0);
}

TEST_F(manager_base_test, mask_operations) {
  {
    // Add single operations
    detail::manager_base mgr{sockets.first, &mpx};
    EXPECT_EQ(mgr.mask(), operation::none);
    EXPECT_TRUE(mgr.mask_add(operation::read));
    EXPECT_EQ(mgr.mask(), operation::read);
    EXPECT_TRUE(mgr.mask_add(operation::write));
    EXPECT_EQ(mgr.mask(), operation::read_write);
    EXPECT_TRUE(mgr.mask_del(operation::write));
    EXPECT_EQ(mgr.mask(), operation::read);
    EXPECT_TRUE(mgr.mask_del(operation::read));
    EXPECT_EQ(mgr.mask(), operation::none);
  }

  {
    // Add multiple operations
    detail::manager_base mgr{sockets.first, &mpx};
    EXPECT_TRUE(mgr.mask_add(operation::read_write));
    EXPECT_EQ(mgr.mask(), operation::read_write);
    EXPECT_TRUE(mgr.mask_del(operation::read_write));
    EXPECT_EQ(mgr.mask(), operation::none);
  }

  {
    // Add partial
    detail::manager_base mgr{sockets.first, &mpx};
    EXPECT_TRUE(mgr.mask_add(operation::read));
    EXPECT_EQ(mgr.mask(), operation::read);
    EXPECT_TRUE(mgr.mask_add(operation::read_write));
    EXPECT_EQ(mgr.mask(), operation::read_write);
  }

  {
    // Delete more than set
    detail::manager_base mgr{sockets.first, &mpx};
    EXPECT_FALSE(mgr.mask_del(operation::read));
    EXPECT_EQ(mgr.mask(), operation::none);
    mgr.mask_set(operation::write);
    EXPECT_TRUE(mgr.mask_del(operation::read_write));
    EXPECT_EQ(mgr.mask(), operation::none);
  }
}
