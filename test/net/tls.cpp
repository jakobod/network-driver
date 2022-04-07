/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "fwd.hpp"

#include "net/multiplexer.hpp"
#include "net/stream_transport.hpp"
#include "net/tls.hpp"
#include "net/transport.hpp"
#include "net/transport_adaptor.hpp"

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
  bool initialized;
  util::byte_buffer received;
  util::const_byte_span data;
  uint64_t handled_timeout;
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

  util::error init() override {
    if (!nonblocking(handle(), true))
      return {util::error_code::runtime_error,
              util::format("Failed to set nonblocking on sock={0}",
                           handle().id)};
    return next_layer_.init();
  }

  event_result handle_read_event() override {
    for (size_t i = 0; i < max_consecutive_reads; ++i) {
      auto read_res = read(handle<stream_socket>(), read_buffer_);
      if (read_res > 0) {
        next_layer_.consume(
          {read_buffer_.data(), static_cast<size_t>(read_res)});
      } else if ((read_res == 0) || !last_socket_error_is_temporary()) {
        return event_result::error;
      }
    }
    return event_result::ok;
  }

  event_result handle_write_event() override {
    auto done_writing = [&]() {
      return !next_layer_.has_more_data() && write_buffer_.empty();
    };
    size_t i = 0;
    while (next_layer_.has_more_data() && (++i <= max_consecutive_fetches))
      next_layer_.produce();
    while (!write_buffer_.empty()) {
      auto res = write(handle<stream_socket>(), write_buffer_);
      EXPECT_GT(res, 0);
      write_buffer_.erase(write_buffer_.begin(), write_buffer_.begin() + res);
    }
    return done_writing() ? event_result::done : event_result::ok;
  }

  event_result handle_timeout(uint64_t id) override {
    return next_layer_.handle_timeout(id);
  }

  NextLayer& next_layer() {
    return next_layer_;
  }

private:
  NextLayer next_layer_;

  transport_vars& vars_;
};

// -- Dummy application layer --------------------------------------------------

struct dummy_application {
  dummy_application(transport_extension& parent, application_vars& vars)
    : parent_(parent), vars_{vars} {
    // nop
  }

  util::error init() {
    vars_.initialized = true;
    return util::none;
  }

  event_result produce() {
    if (vars_.data.empty())
      return event_result::done;
    parent_.enqueue(vars_.data);
    vars_.data = vars_.data.subspan(vars_.data.size());
    return event_result::ok;
  }

  bool has_more_data() const {
    return !vars_.data.empty();
  }

  event_result consume(util::const_byte_span bytes) {
    vars_.received.insert(vars_.received.begin(), bytes.begin(), bytes.end());
    return event_result::ok;
  }

  event_result handle_timeout(uint64_t id) {
    vars_.handled_timeout = id;
    return event_result::ok;
  }

private:
  transport_extension& parent_;
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
    // "Send" client data
    transmit_between(client, server);
    transmit_between(server, client);
  }
}

} // namespace

TEST_F(tls_test, init) {
  const bool is_client = true;
  stack_type stack{sockets.first, nullptr,   transport_vars_,
                   ctx,           is_client, client_application_vars_};
  EXPECT_NO_ERROR(stack.init());
  EXPECT_TRUE(client_application_vars_.initialized);
  EXPECT_EQ(transport_vars_.configured_policy, receive_policy::up_to(2048));
}

TEST_F(tls_test, roundtrip) {
  stack_type client{sockets.first, nullptr, transport_vars_,
                    ctx,           true,    client_application_vars_};
  stack_type server{sockets.second,  nullptr,
                    transport_vars_, ctx,
                    false,           server_application_vars_};
  // Initialize them
  EXPECT_NO_ERROR(client.init());
  EXPECT_NO_ERROR(server.init());
  // Handle the handshake between them
  handle_handshake(client, server);

  EXPECT_TRUE(client.next_layer().next_layer().is_initialized());
  EXPECT_TRUE(server.next_layer().next_layer().is_initialized());

  // TODO
  // Transmit data from client to server and check result
  // transmit_between(client, server);
  // ASSERT_EQ(data.size(), server_application_vars_.received.size());
  // EXPECT_TRUE(std::equal(data.begin(), data.end(),
  //                        server_application_vars_.received.begin()));
}
