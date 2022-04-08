/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "fwd.hpp"

#include "net/ip/v4_endpoint.hpp"

#include "util/error.hpp"

#include "net_test.hpp"

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

constexpr const v4_endpoint any_endpoint{any_address, 22};
constexpr const v4_endpoint localhost_endpoint{localhost_address, 23};
constexpr const v4_endpoint multicast_endpoint{multicast_address, 24};

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

TEST(v4_endpoint_test, to_string) {
  ASSERT_EQ(to_string(any_endpoint), "0.0.0.0:22");
  ASSERT_EQ(to_string(localhost_endpoint), "127.0.0.1:23");
  ASSERT_EQ(to_string(multicast_endpoint), "255.255.255.255:24");
}

TEST(v4_endpoint_test, from_sockaddr) {
  {
    sockaddr_in saddr{};
    saddr.sin_addr.s_addr
      = std::uint32_t{0b00000000'00000000'00000000'00000000};
    saddr.sin_port = htons(22);
    saddr.sin_family = AF_INET;
    v4_endpoint ep{saddr};
    ASSERT_EQ(any_endpoint, ep);
  }
  {
    sockaddr_in saddr{};
    saddr.sin_addr.s_addr
      = std::uint32_t{0b00000001'00000000'00000000'01111111};
    saddr.sin_port = htons(23);
    saddr.sin_family = AF_INET;
    v4_endpoint ep{saddr};
    ASSERT_EQ(localhost_endpoint, ep);
  }
  {
    sockaddr_in saddr{};
    saddr.sin_addr.s_addr
      = std::uint32_t{0b11111111'11111111'11111111'11111111};
    saddr.sin_port = htons(24);
    saddr.sin_family = AF_INET;
    v4_endpoint ep{saddr};
    ASSERT_EQ(multicast_endpoint, ep);
  }
}

TEST(v4_endpoint_test, to_sockaddr) {
  {
    const auto saddr = to_sockaddr_in(any_endpoint);
    ASSERT_EQ(saddr.sin_addr.s_addr, any_endpoint.address().bits());
    ASSERT_EQ(ntohs(saddr.sin_port), any_endpoint.port());
    ASSERT_EQ(saddr.sin_family, AF_INET);
  }
  {
    const auto saddr = to_sockaddr_in(localhost_endpoint);
    ASSERT_EQ(saddr.sin_addr.s_addr, localhost_endpoint.address().bits());
    ASSERT_EQ(ntohs(saddr.sin_port), localhost_endpoint.port());
    ASSERT_EQ(saddr.sin_family, AF_INET);
  }
  {
    const auto saddr = to_sockaddr_in(multicast_endpoint);
    ASSERT_EQ(saddr.sin_addr.s_addr, multicast_endpoint.address().bits());
    ASSERT_EQ(ntohs(saddr.sin_port), multicast_endpoint.port());
    ASSERT_EQ(saddr.sin_family, AF_INET);
  }
}
