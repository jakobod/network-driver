// /**
//  *  @author    Jakob Otto
//  *  @file      stream_transport.cpp
//  *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
//  *             This file is part of the network-driver project, released
//  under
//  *             the GNU GPL3 License.
//  */

// #include "net/detail/datagram_transport.hpp"

// #include "net/detail/event_handler.hpp"
// #if defined(LIB_NET_URING)
// #  include "net/detail/uring_manager.hpp"
// #endif

// #include "net/receive_policy.hpp"
// #include "net/socket/udp_datagram_socket.hpp"

// #include "util/byte_span.hpp"
// #include "util/config.hpp"
// #include "util/error.hpp"
// #include "util/error_or.hpp"

// #include "multiplexer_mock.hpp"
// #include "net_test.hpp"

// #include <algorithm>
// #include <cstring>
// #include <numeric>
// #include <thread>

// using namespace net;

// namespace {

// struct test_data {
//   util::byte_buffer received;
//   util::const_byte_span data;
//   uint64_t last_timeout_id;
// };

// struct dummy_application {
//   dummy_application(detail::transport_base& parent, test_data& data)
//     : parent_{parent}, data_(data) {
//     // nop
//   }

//   util::error init(const util::config&) {
//     parent_.configure_next_read(receive_policy::exactly(1024));
//     return util::none;
//   }

//   event_result produce() {
//     auto size = std::min(size_t{1024}, data_.data.size());
//     parent_.enqueue({data_.data.data(), size});
//     data_.data = data_.data.subspan(size);
//     return event_result::ok;
//   }

//   bool has_more_data() const noexcept { return !data_.data.empty(); }

//   event_result consume(util::const_byte_span data) {
//     data_.received.insert(data_.received.end(), data.begin(), data.end());
//     parent_.configure_next_read(receive_policy::exactly(1024));
//     return event_result::ok;
//   }

//   event_result handle_timeout(uint64_t id) {
//     data_.last_timeout_id = id;
//     return event_result::ok;
//   }

// private:
//   detail::transport_base& parent_;
//   test_data& data_;
// };

// using event_datagram_transport
//   = detail::datagram_transport<detail::event_handler, dummy_application>;

// struct event_datagram_transport_test : public testing::Test, public test_data
// {
//   event_datagram_transport_test() {}

//   ~event_datagram_transport_test() {}
// };

// } // namespace

// #if defined(LIB_NET_URING)

// using uring_stream_transport
//   = detail::stream_transport<detail::uring_manager, dummy_application>;

// struct uring_stream_transport_test : public testing::Test, public test_data {
//   uring_stream_transport_test()
//     : sockets{UNPACK_EXPRESSION(make_stream_socket_pair())},
//       mgr{sockets.first, &mpx, *this} {
//     for (size_t i = 0; i < data_buffer.size(); ++i) {
//       data_buffer[i] = static_cast<std::byte>(i & 0xFF);
//     }
//     data = data_buffer;
//   }

//   ~uring_stream_transport_test() {
//     close(sockets.first);
//     close(sockets.second);
//   }

//   void SetUp() override { ASSERT_EQ(mgr.init(cfg), util::none); }

//   util::byte_array<32768> data_buffer;
//   util::config cfg;
//   stream_socket_pair sockets;
//   multiplexer_mock mpx;
//   uring_stream_transport mgr;
// };

// TEST_F(uring_stream_transport_test, handles_received_data) {
//   // Continuously fill the buffer of the uring_manager with the desired
//   number
//   // of bytes and trigger handling
//   while (!data.empty()) {
//     auto read_buffer = mgr.read_buffer();
//     const auto* ptr = data.data();
//     const auto size = std::min(data.size(), read_buffer.size());
//     std::memcpy(read_buffer.data(), ptr, size);
//     data = data.subspan(size);
//     ASSERT_EQ(mgr.handle_completion(operation::read, static_cast<int>(size)),
//               event_result::ok);
//   }
//   // After writing all data, the application should have consumed all data
//   EXPECT_TRUE(
//     std::equal(received.begin(), received.end(), data_buffer.begin()));
// }

// TEST_F(uring_stream_transport_test, prepares_data_to_write) {
//   static constexpr std::size_t max_retries = 20;
//   size_t received = 0;
//   util::byte_buffer buf;

//   mgr.register_writing();
//   for (std::size_t i = 0; i < max_retries; ++i) {
//     auto write_buffer = mgr.write_buffer();
//     buf.insert(buf.begin(), write_buffer.begin(), write_buffer.end());
//     received += write_buffer.size();
//     const auto res = mgr.handle_completion(operation::write,
//                                            write_buffer.size());
//     ASSERT_NE(res, event_result::error);
//     ASSERT_NE(res, event_result::temporary_error);
//     if (res == event_result::done) {
//       break;
//     }
//   }
//   EXPECT_EQ(received, data_buffer.size());
//   EXPECT_TRUE(std::equal(buf.begin(), buf.end(), data_buffer.begin()));
// }

// TEST_F(uring_stream_transport_test, disconnect) {
//   EXPECT_EQ(mgr.handle_completion(operation::read, 0), event_result::done);
// }

// TEST_F(uring_stream_transport_test, timeout_handling) {
//   ASSERT_EQ(mgr.handle_timeout(42), event_result::ok);
//   EXPECT_EQ(last_timeout_id, 42);
// }

// #endif
