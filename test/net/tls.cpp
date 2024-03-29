/**
 *  @author    Jakob Otto
 *  @file      tls.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/tls.hpp"
#include "net/layer.hpp"
#include "net/multiplexer.hpp"
#include "net/stream_transport.hpp"
#include "net/transport.hpp"
#include "net/transport_adaptor.hpp"

#include "util/config.hpp"
#include "util/error.hpp"
#include "util/format.hpp"

#include "net_test.hpp"

#include <algorithm>
#include <cstring>
#include <numeric>

using namespace net;

namespace {

// Contains variables needed for transport related checks
struct transport_vars {
  bool registered_writing = false;
  receive_policy configured_policy;
};

// Contains variables needed for transport related checks
struct application_vars {
  bool initialized = false;
  bool produce_called = false;
  util::byte_buffer received;
  util::const_byte_span data;
  uint64_t handled_timeout;
};

struct dummy_multiplexer : public multiplexer {
  util::error init(socket_manager_factory_ptr, const util::config&) override {
    return util::none;
  }

  void start() override {
    // nop
  }

  void shutdown() override {
    // nop
  }

  void join() override {
    // nop
  }

  bool running() const override { return false; }

  void handle_error(const util::error& err) override {
    FAIL() << "There should be no errors! " << err << std::endl;
  }

  util::error poll_once(bool) override { return util::none; }

  void add(socket_manager_ptr, operation) override {
    // nop
  }

  void enable(socket_manager_ptr, operation) override {
    // nop
  }

  void disable(socket_manager_ptr, operation, bool) override {
    // nop
  }

  uint64_t set_timeout(socket_manager_ptr,
                       std::chrono::system_clock::time_point) override {
    return 0;
  }
};

template <class NextLayer>
struct dummy_transport : transport {
  template <class... Ts>
  dummy_transport(net::socket handle, multiplexer* mpx, transport_vars& vars,
                  Ts&&... xs)
    : transport(handle, mpx),
      next_layer_(*this, std::forward<Ts>(xs)...),
      vars_{vars} {
    // nop
  }

  /// Configures the amount to be read next
  void configure_next_read(receive_policy policy) override {
    vars_.configured_policy = policy;
    read_buffer_.resize(policy.max_size);
    min_read_size_ = policy.min_size;
  }

  util::error init(const util::config& cfg) override {
    if (auto err = transport::init(cfg))
      return err;
    if (!nonblocking(handle(), true))
      return {util::error_code::runtime_error,
              util::format("Failed to set nonblocking on sock={0}",
                           handle().id)};
    return next_layer_.init(cfg);
  }

  event_result handle_read_event() override {
    for (size_t i = 0; i < transport::max_consecutive_reads_; ++i) {
      auto read_res = read(handle<stream_socket>(), read_buffer_);
      if (read_res > 0) {
        next_layer_.consume(
          {read_buffer_.data(), static_cast<size_t>(read_res)});
      } else if ((read_res == 0)
                 || ((read_res < 0) && !last_socket_error_is_temporary())) {
        return event_result::error;
      }
    }
    return event_result::ok;
  }

  event_result handle_write_event() override {
    event_result res;
    do {
      res = next_layer_.produce();
      EXPECT_NE(res, event_result::error);
    } while (res != event_result::done);

    while (!write_buffer_.empty()) {
      auto res = write(handle<stream_socket>(), write_buffer_);
      EXPECT_GT(res, 0);
      write_buffer_.erase(write_buffer_.begin(), write_buffer_.begin() + res);
    }

    auto done_writing = (!next_layer_.has_more_data() && write_buffer_.empty());
    return done_writing ? event_result::done : event_result::ok;
  }

  event_result handle_timeout(uint64_t id) override {
    return next_layer_.handle_timeout(id);
  }

  NextLayer& next_layer() { return next_layer_; }

private:
  NextLayer next_layer_;

  transport_vars& vars_;
};

// -- Dummy application layer --------------------------------------------------

struct dummy_application {
  dummy_application(layer& parent, application_vars& vars)
    : parent_(parent), vars_{vars} {
    // nop
  }

  util::error init() {
    vars_.initialized = true;
    return util::none;
  }

  event_result produce() {
    vars_.produce_called = true;
    if (vars_.data.empty())
      return event_result::done;
    parent_.enqueue(vars_.data);
    vars_.data = vars_.data.subspan(vars_.data.size());
    return event_result::ok;
  }

  bool has_more_data() const { return !vars_.data.empty(); }

  event_result consume(util::const_byte_span bytes) {
    vars_.received.insert(vars_.received.begin(), bytes.begin(), bytes.end());
    return event_result::ok;
  }

  event_result handle_timeout(uint64_t id) {
    vars_.handled_timeout = id;
    return event_result::ok;
  }

private:
  layer& parent_;
  application_vars& vars_;
};

struct tls_test : public testing::Test {
  using stack_type = dummy_transport<transport_adaptor<tls<dummy_application>>>;

  tls_test() {
    auto maybe_sockets = make_stream_socket_pair();
    EXPECT_EQ(nullptr, util::get_error(maybe_sockets));
    sockets = std::get<stream_socket_pair>(maybe_sockets);
    EXPECT_NO_ERROR(
      ctx.init(CERT_DIRECTORY "/server.crt", CERT_DIRECTORY "/server.key"));

    uint8_t b = 0;
    for (auto& val : data)
      val = std::byte{b++};

    client_application_vars_.data = util::const_byte_span{data};
    server_application_vars_.data = util::const_byte_span{data};
  }

  stream_socket_pair sockets;
  util::byte_array<64> data;
  openssl::tls_context ctx;

  dummy_multiplexer mpx;
  transport_vars transport_vars_;
  application_vars client_application_vars_;
  application_vars server_application_vars_;
};

template <class Stack>
void transmit_between(Stack& lhs, Stack& rhs) {
  EXPECT_NE(lhs.handle_write_event(), event_result::error);
  EXPECT_NE(rhs.handle_read_event(), event_result::error);
};

template <class Stack>
void handle_handshake(Stack& client, Stack& server) {
  // It should take exactly two roundtrips for the handshake to complete
  for (size_t i = 0; i < 2; ++i) {
    transmit_between(client, server);
    transmit_between(server, client);
  }
}

} // namespace

TEST_F(tls_test, init) {
  const bool is_client = true;
  stack_type stack{sockets.first, &mpx,      transport_vars_,
                   ctx,           is_client, client_application_vars_};
  EXPECT_NO_ERROR(stack.init(util::config{}));
  EXPECT_TRUE(client_application_vars_.initialized);
  EXPECT_EQ(transport_vars_.configured_policy, receive_policy::up_to(2048));
}

TEST_F(tls_test, roundtrip) {
  stack_type client{sockets.first, &mpx, transport_vars_,
                    ctx,           true, client_application_vars_};
  stack_type server{sockets.second,          &mpx, transport_vars_, ctx, false,
                    server_application_vars_};
  // Initialize them
  EXPECT_NO_ERROR(client.init(util::config{}));
  EXPECT_NO_ERROR(server.init(util::config{}));

  // Handle the handshake between both peers
  EXPECT_FALSE(client.next_layer().next_layer().handshake_done());
  EXPECT_FALSE(server.next_layer().next_layer().handshake_done());
  handle_handshake(client, server);
  ASSERT_TRUE(client.next_layer().next_layer().handshake_done());
  ASSERT_TRUE(server.next_layer().next_layer().handshake_done());

  // Transmit data from client to server and check result
  transmit_between(client, server);
  EXPECT_TRUE(client_application_vars_.produce_called);
  ASSERT_EQ(data.size(), server_application_vars_.received.size());
  EXPECT_TRUE(std::equal(data.begin(), data.end(),
                         server_application_vars_.received.begin()));
}
