/**
 *  @author    Jakob Otto
 *  @file      v4_address.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/ip/v4_address.hpp"

#include "util/error.hpp"
#include "util/error_or.hpp"

#include <charconv>
#include <ranges>
#include <string_view>

using namespace std::string_view_literals;

namespace net::ip {

bool operator==(const v4_address& lhs, const v4_address& rhs) {
  return lhs.bits() == rhs.bits();
}

bool operator!=(const v4_address& lhs, const v4_address& rhs) {
  return !(lhs == rhs);
}

std::string to_string(const v4_address& addr) {
  return std::format("{}.{}.{}.{}", static_cast<std::uint8_t>(addr.bytes()[0]),
                     static_cast<std::uint8_t>(addr.bytes()[1]),
                     static_cast<std::uint8_t>(addr.bytes()[2]),
                     static_cast<std::uint8_t>(addr.bytes()[3]));
}

util::error_or<v4_address> parse_v4_address(const std::string_view str) {
  v4_address::octet_array bytes;
  auto it = bytes.begin();
  for (const auto& part : std::views::split(str, "."sv)) {
    std::uint8_t octet = 0;
    auto [_, ec] = std::from_chars(part.data(), part.data() + part.size(),
                                   octet);
    if (ec != std::errc{}) {
      return util::error{util::error_code::parser_error,
                         "failed to parse octet"};
    }
    *it++ = std::byte{octet};
  }
  if (it != bytes.end()) {
    return util::error{util::error_code::parser_error,
                       "not enough octets in address string"};
  }
  return bytes;
}

} // namespace net::ip
