// /**
//  *  @author    Jakob Otto
//  *  @file      stream_transport.cpp
//  *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
//  *             This file is part of the network-driver project, released
//  under
//  *             the GNU GPL3 License.
//  */

// #include "net/datagram_transport.hpp"

// #include "net/receive_policy.hpp"
// #include "net/socket/datagram_socket.hpp"

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

// struct dummy_application {
//   dummy_application(detail::transport_base& parent, util::const_byte_span
//   data,
//                     util::byte_buffer& received, uint64_t& last_timeout_id)
//     : received_(received),
//       data_(data),
//       last_timeout_id_(last_timeout_id),
//       parent_(parent) {
//     // nop
//   }

//   util::error init(const util::config&) {
//     parent_.configure_next_read(receive_policy::exactly(1024));
//     return util::none;
//   }

//   manager_result produce() {
//     auto size = std::min(size_t{1024}, data_.size());
//     parent_.enqueue({data_.data(), size});
//     data_ = data_.subspan(size);
//     return manager_result::ok;
//   }

//   bool has_more_data() { return !data_.empty(); }

//   manager_result consume(util::const_byte_span data) {
//     received_.insert(received_.end(), data.begin(), data.end());
//     parent_.configure_next_read(receive_policy::exactly(1024));
//     return manager_result::ok;
//   }

//   manager_result handle_timeout(uint64_t id) {
//     last_timeout_id_ = id;
//     return manager_result::ok;
//   }

// private:
//   util::byte_buffer& received_;
//   util::const_byte_span data_;
//   uint64_t& last_timeout_id_;

//   transport& parent_;
// };

// using manager_type = datagram_transport<dummy_application>;

// struct datagram_transport_test : public testing::Test {
//   datagram_transport_test()
//     : sockets{stream_socket{invalid_socket_id},
//               stream_socket{invalid_socket_id}} {
//     sockets = UNPACK_EXPRESSION(make_stream_socket_pair());
//     uint8_t b = 0;
//     for (auto& val :
//          std::span{reinterpret_cast<uint8_t*>(data.data()), data.size()}) {
//       val = b++;
//     }
//   }

//   util::config cfg{};
//   stream_socket_pair sockets;
//   multiplexer_mock mpx;
//   util::byte_array<32768> data;

//   util::byte_buffer received_data;
//   uint64_t last_timeout_id;
// };

// void write_all_data(net::stream_socket sock, util::const_byte_span data) {
//   while (!data.empty()) {
//     const auto res = write(sock, data);
//     if (res > 0) {
//       data = data.subspan(res);
//     } else if (!last_socket_error_is_temporary()) {
//       FAIL() << "Socket error";
//       return;
//     }
//   }
// }

// } // namespace

// TEST_F(datagram_transport_test, handle_read_event) {
//   manager_type mgr(sockets.first, &mpx, std::span{data}, received_data,
//                    last_timeout_id);
//   ASSERT_EQ(mgr.init(cfg), util::none);
//   std::thread writer([this] { write_all_data(sockets.second, data); });
//   while (received_data.size() < data.size()) {
//     ASSERT_EQ(mgr.handle_read_event(), manager_result::ok);
//   }
//   EXPECT_TRUE(
//     std::equal(received_data.begin(), received_data.end(), data.begin()));
//   if (writer.joinable()) {
//     writer.join();
//   }
// }

// TEST_F(datagram_transport_test, handle_write_event) {
//   size_t received = 0;
//   util::byte_array<32768> buf;
//   manager_type mgr(sockets.first, &mpx, std::span{data}, received_data,
//                    last_timeout_id);
//   ASSERT_EQ(mgr.init(cfg), util::none);
//   auto read_some = [&]() {
//     auto data = buf.data() + received;
//     auto remaining = buf.size() - received;
//     auto res = read(sockets.second, std::span{data, remaining});
//     if (res < 0) {
//       ASSERT_TRUE(last_socket_error_is_temporary());
//     } else if (res == 0) {
//       FAIL() << "socket diconnected prematurely!" << std::endl;
//     }
//     received += res;
//   };
//   while (mgr.handle_write_event() == manager_result::ok) {
//     read_some();
//   }
//   read_some();
//   ASSERT_EQ(received, data.size());
//   EXPECT_EQ(memcmp(data.data(), received_data.data(), received_data.size()),
//   0);
// }

// TEST_F(datagram_transport_test, disconnect) {
//   manager_type mgr(sockets.first, &mpx, std::span{data}, received_data,
//                    last_timeout_id);
//   ASSERT_EQ(mgr.init(cfg), util::none);
//   close(sockets.second);
//   EXPECT_EQ(mgr.handle_read_event(), manager_result::done);
// }

// TEST_F(datagram_transport_test, timeout_handling) {
//   manager_type mgr(sockets.first, &mpx, std::span{data}, received_data,
//                    last_timeout_id);
//   ASSERT_NO_ERROR(mgr.init(cfg));
//   EXPECT_EQ(mgr.handle_timeout(42), manager_result::ok);
// }
