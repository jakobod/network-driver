// /**
//  *  @author    Jakob Otto
//  *  @file      transport_adaptor.cpp
//  *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
//  *             This file is part of the network-driver project, released
//  under
//  *             the GNU GPL3 License.
//  */

// #include "net/detail/transport_adaptor.hpp"

// #include "net/detail/layer_base.hpp"
// #include "net/detail/multiplexer_base.hpp"
// #include "net/detail/stream_transport.hpp"
// #include "net/detail/transport_base.hpp"

// #include "net/receive_policy.hpp"

// #include "util/config.hpp"
// #include "util/error.hpp"
// #include "util/error_or.hpp"
// #include "util/format.hpp"

// #include "net_test.hpp"

// #include <algorithm>
// #include <cstring>
// #include <numeric>

// using namespace net;

// namespace {

// // Contains variables needed for transport related checks
// struct transport_vars {
//   bool registered_writing;
//   util::const_byte_span data;
//   receive_policy configured_policy;
// };

// // Contains variables needed for application related checks
// struct application_vars {
//   bool initialized;
//   util::byte_buffer received;
//   util::const_byte_span data;
//   uint64_t handled_timeout;
// };

// template <class NextLayer>
// struct dummy_transport : public detail::transport_base {
//   template <class... Ts>
//   dummy_transport(net::socket handle, detail::multiplexer_base* mpx,
//                   transport_vars& vars, Ts&&... xs)
//     : detail::transport_base(handle, mpx),
//       next_layer_(*this, std::forward<Ts>(xs)...),
//       vars_{vars} {
//     // nop
//   }

//   /// Configures the amount to be read next
//   void configure_next_read(receive_policy policy) noexcept override {
//     vars_.configured_policy = policy;
//   }

//   util::error init(const util::config& cfg) override {
//     if (auto err = transport_base::init(cfg)) {
//       return err;
//     }
//     if (!nonblocking(handle(), true)) {
//       return {util::error_code::runtime_error,
//               "Failed to set nonblocking on sock={0}", handle().id};
//     }
//     return next_layer_.init(cfg);
//   }

//   manager_result handle_read_event() override {
//     next_layer_.consume(vars_.data);
//     return manager_result::ok;
//   }

//   manager_result handle_write_event() override {
//     auto done_writing = [&]() {
//       return !next_layer_.has_more_data() && write_buffer_.empty();
//     };
//     size_t i = 0;
//     while (next_layer_.has_more_data()
//            && (++i <= transport_base::max_consecutive_fetches_)) {
//       next_layer_.produce();
//     }
//     while (!write_buffer_.empty()) {
//       auto res = write(handle<stream_socket>(), write_buffer_);
//       EXPECT_GT(res, 0);
//       write_buffer_.erase(write_buffer_.begin(), write_buffer_.begin() +
//       res);
//     }
//     return done_writing() ? manager_result::done : manager_result::ok;
//   }

//   // manager_result handle_timeout(uint64_t id) override {
//   //   return next_layer_.handle_timeout(id);
//   // }

// private:
//   NextLayer next_layer_;

//   transport_vars& vars_;
// };

// // -- Dummy application layer
// --------------------------------------------------

// struct dummy_application {
//   dummy_application(detail::layer_base& parent, application_vars& vars)
//     : parent_(parent), vars_{vars} {
//     // nop
//   }

//   util::error init(const util::config&) {
//     vars_.initialized = true;
//     parent_.configure_next_read(receive_policy::exactly(1024));
//     return util::none;
//   }

//   manager_result produce() {
//     if (vars_.data.empty()) {
//       return manager_result::done;
//     }
//     parent_.enqueue(vars_.data);
//     vars_.data = vars_.data.subspan(vars_.data.size());
//     return manager_result::ok;
//   }

//   bool has_more_data() const { return !vars_.data.empty(); }

//   manager_result consume(util::const_byte_span bytes) {
//     vars_.received.insert(vars_.received.begin(), bytes.begin(),
//     bytes.end()); return manager_result::ok;
//   }

//   manager_result handle_timeout(uint64_t id) {
//     vars_.handled_timeout = id;
//     return manager_result::ok;
//   }

// private:
//   detail::layer_base& parent_;
//   application_vars& vars_;
// };

// struct transport_adaptor_test : public testing::Test {
//   using stack_type = dummy_transport<transport_adaptor<dummy_application>>;

//   transport_adaptor_test() {
//     sockets = UNPACK_EXPRESSION(make_stream_socket_pair());

//     uint8_t b = 0;
//     for (auto& val : data) {
//       val = std::byte{b++};
//     }

//     transport_vars_.data = util::const_byte_span{data};
//     application_vars_.data = util::const_byte_span{data};
//   }

//   stream_socket_pair sockets;
//   util::byte_array<1024> data;

//   transport_vars transport_vars_;
//   application_vars application_vars_;
// };

// } // namespace

// TEST_F(transport_adaptor_test, init) {
//   stack_type stack{sockets.first, nullptr, transport_vars_,
//   application_vars_}; EXPECT_NO_ERROR(stack.init(util::config{}));
//   EXPECT_TRUE(application_vars_.initialized);
//   EXPECT_EQ(transport_vars_.configured_policy,
//   receive_policy::exactly(1024));
// }

// TEST_F(transport_adaptor_test, handle_read_event) {
//   stack_type stack{sockets.first, nullptr, transport_vars_,
//   application_vars_}; EXPECT_NO_ERROR(stack.init(util::config{}));
//   EXPECT_EQ(data.size(), write(sockets.second, data));
//   while (application_vars_.received.size() < data.size()) {
//     ASSERT_EQ(stack.handle_read_event(), manager_result::ok);
//   }
//   EXPECT_EQ(data.size(), application_vars_.received.size());
//   EXPECT_TRUE(
//     std::equal(data.begin(), data.end(),
//     application_vars_.received.begin()));
// }

// TEST_F(transport_adaptor_test, handle_write_event) {
//   stack_type stack{sockets.first, nullptr, transport_vars_,
//   application_vars_}; EXPECT_NO_ERROR(stack.init(util::config{})); while
//   (stack.handle_write_event() == manager_result::ok)
//     ;
//   util::byte_array<1024> receive_buffer;
//   util::byte_span free_space{receive_buffer};
//   auto read_some = [&]() {
//     auto res = read(sockets.second, free_space);
//     if (res < 0) {
//       ASSERT_TRUE(last_socket_error_is_temporary());
//     } else if (res == 0) {
//       FAIL() << "socket diconnected prematurely!" << std::endl;
//     }
//     free_space = free_space.subspan(res);
//   };
//   size_t rounds = 0;
//   while (!free_space.empty() && (++rounds < 20)) {
//     read_some();
//   }
//   EXPECT_TRUE(std::equal(data.begin(), data.end(), receive_buffer.begin()));
// }

// // TEST_F(transport_adaptor_test, handle_timeout) {
// //   stack_type stack{sockets.first, nullptr, transport_vars_,
// //   application_vars_}; EXPECT_NO_ERROR(stack.init(util::config{}));
// //   EXPECT_EQ(stack.handle_timeout(42), manager_result::ok);
// //   EXPECT_EQ(application_vars_.handled_timeout, 42);
// // }
