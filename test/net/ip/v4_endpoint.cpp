/**
 *  @author    Jakob Otto
 *  @file      v4_endpoint.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/ip/v4_endpoint.hpp"

#include "util/error.hpp"
#include "util/error_or.hpp"

#include "net_test.hpp"

#include <netinet/ip.h>

using namespace net;
using namespace net::ip;

using namespace std::string_literals;

namespace {

constexpr const v4_address any_address{v4_address::any};
constexpr const v4_address localhost_address{v4_address::localhost};
constexpr const v4_address broadcast_address{v4_address::local_broadcast};

constexpr const v4_endpoint any_endpoint{any_address, 22};
constexpr const v4_endpoint localhost_endpoint{localhost_address, 23};
constexpr const v4_endpoint broadcast_endpoint{broadcast_address, 24};

} // namespace

TEST(v4_endpoint_test, parse) {
  {
    auto maybe_endpoint = parse_v4_endpoint("0.0.0.0:0");
    ASSERT_EQ(nullptr, util::get_error(maybe_endpoint));
    auto ep = std::get<v4_endpoint>(maybe_endpoint);
    ASSERT_EQ(any_address, ep.address());
    ASSERT_EQ(std::uint16_t{0}, ep.port());
  }
  {
    auto maybe_endpoint = parse_v4_endpoint("127.0.0.1:12345");
    ASSERT_EQ(nullptr, util::get_error(maybe_endpoint));
    auto ep = std::get<v4_endpoint>(maybe_endpoint);
    ASSERT_EQ(localhost_address, ep.address());
    ASSERT_EQ(std::uint16_t{12345}, ep.port());
  }
  {
    auto maybe_endpoint = parse_v4_endpoint("255.255.255.255:22");
    ASSERT_EQ(nullptr, util::get_error(maybe_endpoint));
    auto ep = std::get<v4_endpoint>(maybe_endpoint);
    ASSERT_EQ(broadcast_address, ep.address());
    ASSERT_EQ(std::uint16_t{22}, ep.port());
  }
}

TEST(v4_endpoint_test, to_string) {
  ASSERT_EQ(to_string(any_endpoint), "0.0.0.0:22");
  ASSERT_EQ(to_string(localhost_endpoint), "127.0.0.1:23");
  ASSERT_EQ(to_string(broadcast_endpoint), "255.255.255.255:24");
}

TEST(v4_endpoint_test, from_sockaddr) {
  {
    sockaddr_in saddr{};
    saddr.sin_addr.s_addr
      = std::uint32_t{0b00000000'00000000'00000000'00000000};
    saddr.sin_port = htons(22);
    saddr.sin_family = AF_INET;
    const v4_endpoint ep{saddr};
    ASSERT_EQ(any_endpoint, ep);
  }
  {
    sockaddr_in saddr{};
    saddr.sin_addr.s_addr
      = std::uint32_t{0b00000001'00000000'00000000'01111111};
    saddr.sin_port = htons(23);
    saddr.sin_family = AF_INET;
    const v4_endpoint ep{saddr};
    ASSERT_EQ(localhost_endpoint, ep);
  }
  {
    sockaddr_in saddr{};
    saddr.sin_addr.s_addr
      = std::uint32_t{0b11111111'11111111'11111111'11111111};
    saddr.sin_port = htons(24);
    saddr.sin_family = AF_INET;
    const v4_endpoint ep{saddr};
    ASSERT_EQ(broadcast_endpoint, ep);
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
    const auto saddr = to_sockaddr_in(broadcast_endpoint);
    ASSERT_EQ(saddr.sin_addr.s_addr, broadcast_endpoint.address().bits());
    ASSERT_EQ(ntohs(saddr.sin_port), broadcast_endpoint.port());
    ASSERT_EQ(saddr.sin_family, AF_INET);
  }
}
