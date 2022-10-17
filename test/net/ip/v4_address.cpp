/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/ip/v4_address.hpp"

#include "util/error.hpp"
#include "util/error_or.hpp"

#include "net_test.hpp"

using namespace net;
using namespace net::ip;

using namespace std::string_literals;

namespace {

constexpr const v4_address any_address{v4_address::any};
constexpr const v4_address localhost_address{v4_address::localhost};
constexpr const v4_address broadcast_address{v4_address::local_broadcast};

} // namespace

namespace net::ip {

bool operator==(const v4_address::octet_array& lhs,
                const v4_address::octet_array& rhs) {
  return (lhs.size() == rhs.size())
         && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

} // namespace net::ip

TEST(v4_address_test, init) {
  ASSERT_EQ(v4_address::any, any_address.bytes());
  ASSERT_EQ(std::uint32_t{0b00000000'00000000'00000000'00000000},
            any_address.bits());

  ASSERT_EQ(v4_address::localhost, localhost_address.bytes());
  ASSERT_EQ(std::uint32_t{0b00000001'00000000'00000000'01111111},
            localhost_address.bits());

  ASSERT_EQ(v4_address::local_broadcast, broadcast_address.bytes());
  ASSERT_EQ(std::uint32_t{0b11111111'11111111'11111111'11111111},
            broadcast_address.bits());
}

TEST(v4_address_test, toString) {
  ASSERT_EQ(to_string(any_address), "0.0.0.0"s);
  ASSERT_EQ(to_string(localhost_address), "127.0.0.1"s);
  ASSERT_EQ(to_string(broadcast_address), "255.255.255.255"s);
}

TEST(v4_address_test, parse) {
  {
    auto maybe_addr = parse_v4_address("0.0.0.0");
    ASSERT_EQ(nullptr, util::get_error(maybe_addr));
    ASSERT_EQ(any_address, std::get<v4_address>(maybe_addr));
  }
  {
    auto maybe_addr = parse_v4_address("127.0.0.1");
    ASSERT_EQ(nullptr, util::get_error(maybe_addr));
    ASSERT_EQ(localhost_address, std::get<v4_address>(maybe_addr));
  }
  {
    auto maybe_addr = parse_v4_address("255.255.255.255");
    ASSERT_EQ(nullptr, util::get_error(maybe_addr));
    ASSERT_EQ(broadcast_address, std::get<v4_address>(maybe_addr));
  }
}

TEST(v4_address_test, parsing_fails) {
  {
    auto maybe_addr = parse_v4_address("0.0.0");
    ASSERT_NE(nullptr, get_error(maybe_addr));
  }
  {
    auto maybe_addr = parse_v4_address("0:0:0:674");
    ASSERT_NE(nullptr, get_error(maybe_addr));
  }
}
