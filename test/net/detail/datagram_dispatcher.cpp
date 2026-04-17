/**
 *  @author    Jakob Otto
 *  @file      datagram_dispatcher.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released
 under
 *             the GNU GPL3 License.
 */

#include "net/detail/datagram_dispatcher.hpp"

#include "net/ip/v4_endpoint.hpp"

#include "util/byte_literals.hpp"

#include "net_test.hpp"

using namespace net;
using namespace net::ip;
using namespace net::detail;
using namespace util::byte_literals;

namespace {

struct test_state {
  std::size_t factory_calls{0};
  std::size_t init_calls{0};
  std::size_t produce_calls{0};
  std::size_t has_more_data_calls{0};
  std::size_t consume_calls{0};
  std::size_t handle_timeout_calls{0};
  std::vector<v4_endpoint> enqueued_endpoints;
};

struct application_mock {
  application_mock(test_state& state) : state_{state} {}

  util::error init(auto& parent, const util::config&) {
    ++state_.init_calls;
    parent.set_timeout_in(10ms);
    parent.set_timeout_at(std::chrono::steady_clock::now() + 10ms);
    return util::none;
  }

  manager_result produce(auto& parent) {
    ++state_.produce_calls;
    parent.enqueue(util::const_byte_span{});
    return manager_result::ok;
  }

  bool has_more_data() const noexcept {
    ++state_.has_more_data_calls;
    return false;
  }

  manager_result consume(auto&, util::const_byte_span) {
    ++state_.consume_calls;
    return manager_result::ok;
  }

  manager_result handle_timeout(auto&, uint64_t) {
    ++state_.handle_timeout_calls;
    return manager_result::ok;
  }

private:
  test_state& state_;
};

using dispatcher_type = datagram_dispatcher<application_mock>;

struct datagram_dispatcher_test : public testing::Test {
  dispatcher_type make_dispatcher() {
    auto factory = [this] {
      ++state.factory_calls;
      return application_mock{state};
    };
    dispatcher_type disp{std::move(factory)};
    EXPECT_NO_ERROR(disp.init(*this, cfg));
    return disp;
  }

  void enqueue(util::const_byte_span, v4_endpoint ep) {
    state.enqueued_endpoints.push_back(ep);
  }

  std::uint64_t set_timeout_in(std::chrono::steady_clock::duration) {
    return timeout_in_id;
  }

  std::uint64_t set_timeout_at(std::chrono::steady_clock::time_point) {
    return timeout_at_id;
  }

  util::config cfg;
  test_state state;
  static constexpr std::uint64_t timeout_in_id = 1;
  static constexpr std::uint64_t timeout_at_id = 1;
};

} // namespace

TEST_F(datagram_dispatcher_test, init) {
  auto disp = make_dispatcher();
  EXPECT_EQ(state.factory_calls, 0ull);
  EXPECT_EQ(state.init_calls, 0ull);
  EXPECT_EQ(state.produce_calls, 0ull);
  EXPECT_EQ(state.has_more_data_calls, 0ull);
  EXPECT_EQ(state.consume_calls, 0ull);
  EXPECT_EQ(state.handle_timeout_calls, 0ull);
  EXPECT_EQ(state.enqueued_endpoints.size(), 0ull);
}

TEST_F(datagram_dispatcher_test, consume) {
  auto disp = make_dispatcher();
  v4_endpoint ep{v4_address::localhost, 12345};
  EXPECT_EQ(disp.consume(*this, util::byte_span{}, ep), manager_result::ok);
  EXPECT_EQ(state.factory_calls, 1ull);
  EXPECT_EQ(state.init_calls, 1ull);
  EXPECT_EQ(state.produce_calls, 0ull);
  EXPECT_EQ(state.has_more_data_calls, 0ull);
  EXPECT_EQ(state.consume_calls, 1ull);
  EXPECT_EQ(state.handle_timeout_calls, 0ull);
  EXPECT_EQ(state.enqueued_endpoints.size(), 0ull);
}

TEST_F(datagram_dispatcher_test, produce) {
  auto disp = make_dispatcher();
  v4_endpoint ep{v4_address::localhost, 12345};
  EXPECT_EQ(disp.consume(*this, util::byte_span{}, ep), manager_result::ok);
  // Now a single application is contained in the dispatcher, try producing
  EXPECT_EQ(disp.produce(*this), manager_result::ok);

  EXPECT_EQ(state.factory_calls, 1ull);
  EXPECT_EQ(state.init_calls, 1ull);
  EXPECT_EQ(state.produce_calls, 1ull);
  EXPECT_EQ(state.has_more_data_calls, 0ull);
  EXPECT_EQ(state.consume_calls, 1ull);
  EXPECT_EQ(state.handle_timeout_calls, 0ull);
  EXPECT_EQ(state.enqueued_endpoints.size(), 1ull);
}

TEST_F(datagram_dispatcher_test, set_timeout_in) {
  auto disp = make_dispatcher();
  v4_endpoint ep{v4_address::localhost, 12345};
  EXPECT_EQ(disp.consume(*this, util::byte_span{}, ep), manager_result::ok);
  EXPECT_EQ(disp.handle_timeout(*this, timeout_in_id), manager_result::ok);

  EXPECT_EQ(state.factory_calls, 1ull);
  EXPECT_EQ(state.init_calls, 1ull);
  EXPECT_EQ(state.produce_calls, 0ull);
  EXPECT_EQ(state.has_more_data_calls, 0ull);
  EXPECT_EQ(state.consume_calls, 1ull);
  EXPECT_EQ(state.handle_timeout_calls, 1ull);
  EXPECT_EQ(state.enqueued_endpoints.size(), 0ull);
}

TEST_F(datagram_dispatcher_test, set_timeout_at) {
  auto disp = make_dispatcher();
  v4_endpoint ep{v4_address::localhost, 12345};
  EXPECT_EQ(disp.consume(*this, util::byte_span{}, ep), manager_result::ok);
  EXPECT_EQ(disp.handle_timeout(*this, timeout_at_id), manager_result::ok);

  EXPECT_EQ(state.factory_calls, 1ull);
  EXPECT_EQ(state.init_calls, 1ull);
  EXPECT_EQ(state.produce_calls, 0ull);
  EXPECT_EQ(state.has_more_data_calls, 0ull);
  EXPECT_EQ(state.consume_calls, 1ull);
  EXPECT_EQ(state.handle_timeout_calls, 1ull);
  EXPECT_EQ(state.enqueued_endpoints.size(), 0ull);
}
