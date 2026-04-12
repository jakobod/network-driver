/**
 *  @author    Jakob Otto
 *  @file      acceptor.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/detail/acceptor.hpp"

#include "net/detail/multiplexer_base.hpp"

#include "net/manager_result.hpp"
#include "net/operation.hpp"
#include "net/socket_guard.hpp"

#include "net/ip/v4_address.hpp"
#include "net/ip/v4_endpoint.hpp"

#include "net/socket/tcp_accept_socket.hpp"
#include "net/socket/tcp_stream_socket.hpp"

#include "util/error.hpp"
#include "util/error_or.hpp"

#include "net_test.hpp"

using namespace net;
using namespace net::ip;

namespace {

class multiplexer_mock : public net::detail::multiplexer_base {
public:
  MOCK_METHOD(void, add, (detail::manager_base_ptr, operation), (override));

private:
  MOCK_METHOD(manager_map::iterator, del, (manager_map::iterator), (override));

public:
  MOCK_METHOD(void, enable, (detail::manager_base&, operation), (override));
  MOCK_METHOD(void, disable, (detail::manager_base&, operation, bool),
              (override));
  MOCK_METHOD(util::error, poll_once, (bool), (override));
  MOCK_METHOD(void, handle_error, (util::error), (override));
  MOCK_METHOD(void, shutdown, (), (override));
};

class uring_manager_mock : public detail::uring_manager {
public:
  using detail::uring_manager::uring_manager;

  MOCK_METHOD(manager_result, enable, (operation), (override));
  MOCK_METHOD(manager_result, handle_completion, (operation, int), (override));
};

} // namespace

TEST(event_handler_acceptor_test, handle_read_event) {
  multiplexer_mock mpx;
  EXPECT_CALL(mpx, add(testing::NotNull(), operation::read)).Times(1);
  auto [accept_socket, port] = UNPACK_EXPRESSION(
    make_tcp_accept_socket({net::ip::v4_address::localhost, 0}));
  bool factory_called = false;
  auto factory = [&factory_called,
                  &mpx](net::socket handle) -> detail::manager_base_ptr {
    factory_called = true;
    return util::make_intrusive<detail::event_handler>(handle, &mpx);
  };

  detail::event_handler_acceptor acceptor{accept_socket, &mpx,
                                          std::move(factory)};
  net::socket_guard sock{
    UNPACK_EXPRESSION(net::make_connected_tcp_stream_socket(
      v4_endpoint{v4_address::localhost, port}))};

  test::invoke([&] {
    EXPECT_EQ(acceptor.handle_read_event(), manager_result::ok);
  }).until([&factory_called] { return factory_called; });
  EXPECT_TRUE(factory_called);
}

#if defined(LIB_NET_URING)

TEST(uring_acceptor_test, handle_completion_rejects_invalid_operations) {
  multiplexer_mock mpx;
  EXPECT_CALL(mpx, add(testing::NotNull(), operation::read)).Times(0);
  detail::uring_acceptor acceptor{tcp_accept_socket{invalid_socket_id}, &mpx,
                                  [](net::socket) -> detail::manager_base_ptr {
                                    return nullptr;
                                  }};
  EXPECT_EQ(acceptor.handle_completion(operation::read, 42),
            manager_result::error);
  EXPECT_EQ(acceptor.handle_completion(operation::write, 42),
            manager_result::error);
  EXPECT_EQ(acceptor.handle_completion(operation::none, 42),
            manager_result::error);
  EXPECT_EQ(acceptor.handle_completion(operation::poll_read, 42),
            manager_result::error);
  EXPECT_EQ(acceptor.handle_completion(operation::poll_write, 42),
            manager_result::error);
}

TEST(uring_acceptor_test, handle_completion_handles_error) {
  multiplexer_mock mpx;
  EXPECT_CALL(mpx, add(testing::NotNull(), operation::read)).Times(0);
  detail::uring_acceptor acceptor{tcp_accept_socket{invalid_socket_id}, &mpx,
                                  [](net::socket) -> detail::manager_base_ptr {
                                    return nullptr;
                                  }};
  EXPECT_EQ(acceptor.handle_completion(operation::read, -1),
            manager_result::error);
  EXPECT_EQ(acceptor.handle_completion(operation::write, 0),
            manager_result::error);
}

TEST(uring_acceptor_test, handle_completion) {
  multiplexer_mock mpx;
  EXPECT_CALL(mpx, add(testing::NotNull(), operation::read)).Times(1);
  auto [accept_socket, port] = UNPACK_EXPRESSION(
    make_tcp_accept_socket({net::ip::v4_address::localhost, 0}));
  bool factory_called = false;
  auto factory = [&factory_called,
                  &mpx](net::socket handle) -> detail::manager_base_ptr {
    factory_called = true;
    return util::make_intrusive<uring_manager_mock>(handle, &mpx);
  };

  detail::uring_acceptor acceptor{accept_socket, &mpx, std::move(factory)};
  EXPECT_EQ(acceptor.handle_completion(operation::accept, 42),
            manager_result::ok);
  EXPECT_TRUE(factory_called);
}

#endif
