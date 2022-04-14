/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "fwd.hpp"

#include "net/ip/v4_address.hpp"

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

} // namespace

namespace net::ip {

bool operator==(const v4_address::octet_array& lhs,
                const v4_address::octet_array& rhs) {
  return (lhs.size() == rhs.size())
         && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

} // namespace net::ip

TEST(v4_address_test, init) {
  ASSERT_EQ(unset_bytes, any_address.bytes());
  ASSERT_EQ(std::uint32_t{0b00000000'00000000'00000000'00000000},
            any_address.bits());

  ASSERT_EQ(localhost_bytes, localhost_address.bytes());
  ASSERT_EQ(std::uint32_t{0b00000001'00000000'00000000'01111111},
            localhost_address.bits());

  ASSERT_EQ(multicast_bytes, multicast_address.bytes());
  ASSERT_EQ(std::uint32_t{0b11111111'11111111'11111111'11111111},
            multicast_address.bits());
}

TEST(v4_address_test, toString) {
  ASSERT_EQ(to_string(unset_bytes), "0.0.0.0"s);
  ASSERT_EQ(to_string(localhost_address), "127.0.0.1"s);
  ASSERT_EQ(to_string(multicast_address), "255.255.255.255"s);
}

TEST(v4_address_test, parse) {
  {
    auto maybe_addr = parse_v4_address("0.0.0.0");
    ASSERT_EQ(nullptr, get_error(maybe_addr));
    ASSERT_EQ(any_address, std::get<v4_address>(maybe_addr));
  }
  {
    auto maybe_addr = parse_v4_address("127.0.0.1");
    ASSERT_EQ(nullptr, get_error(maybe_addr));
    ASSERT_EQ(localhost_address, std::get<v4_address>(maybe_addr));
  }
  {
    auto maybe_addr = parse_v4_address("255.255.255.255");
    ASSERT_EQ(nullptr, get_error(maybe_addr));
    ASSERT_EQ(multicast_address, std::get<v4_address>(maybe_addr));
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
