/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "fwd.hpp"

#include "net/ip/v4_endpoint.hpp"

#include "net/error.hpp"

#include <gtest/gtest.h>

using namespace net;
using namespace net::ip;

using namespace std::string_literals;

namespace {

constexpr const auto unset_bytes = util::make_byte_array(0, 0, 0, 0);
constexpr const auto localhost_bytes = util::make_byte_array(127, 0, 0, 1);
constexpr const auto multicast_bytes = util::make_byte_array(255, 255, 255,
                                                             255);

constexpr const v4_address any_address{unset_bytes};
constexpr const v4_address localhost_address{localhost_bytes};
constexpr const v4_address multicast_address{multicast_bytes};

} // namespace

TEST(v4_endpoint_test, parse) {
  {
    auto maybe_endpoint = parse_v4_endpoint("0.0.0.0:0");
    ASSERT_EQ(nullptr, get_error(maybe_endpoint));
    auto ep = std::get<v4_endpoint>(maybe_endpoint);
    ASSERT_EQ(any_address, ep.address());
    ASSERT_EQ(std::uint16_t{0}, ep.port());
  }
  {
    auto maybe_endpoint = parse_v4_endpoint("127.0.0.1:12345");
    ASSERT_EQ(nullptr, get_error(maybe_endpoint));
    auto ep = std::get<v4_endpoint>(maybe_endpoint);
    ASSERT_EQ(localhost_address, ep.address());
    ASSERT_EQ(std::uint16_t{12345}, ep.port());
  }
  {
    auto maybe_endpoint = parse_v4_endpoint("255.255.255.255:22");
    ASSERT_EQ(nullptr, get_error(maybe_endpoint));
    auto ep = std::get<v4_endpoint>(maybe_endpoint);
    ASSERT_EQ(multicast_address, ep.address());
    ASSERT_EQ(std::uint16_t{22}, ep.port());
  }
}
