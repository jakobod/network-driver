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

#include <charconv>
#include <ranges>
#include <string_view>

using namespace std::string_view_literals;

namespace net::ip {

v4_endpoint::v4_endpoint(sockaddr_in saddr)
  : address_{saddr.sin_addr.s_addr}, port_{ntohs(saddr.sin_port)} {
  // nop
}

bool operator==(const v4_endpoint& lhs, const v4_endpoint& rhs) {
  return (lhs.address() == rhs.address()) && (lhs.port() == rhs.port());
}

bool operator!=(const v4_endpoint& lhs, const v4_endpoint& rhs) {
  return !(lhs == rhs);
}

std::string to_string(const v4_endpoint& ep) {
  return std::format("{}:{}", to_string(ep.address()), ep.port());
}

util::error_or<v4_endpoint> parse_v4_endpoint(const std::string& str) {
  const auto separator_pos = str.find_first_of(':');
  if (separator_pos == std::string::npos) {
    return util::error{util::error_code::parser_error,
                       "Parsing to v4_endpoint failed: needs address and port, "
                       "separated by ':'"};
  }
  const auto maybe_addr = parse_v4_address(str.substr(0, separator_pos));
  if (auto err = util::get_error(maybe_addr)) {
    return *err;
  }
  const auto port_string = str.substr(separator_pos + 1);
  std::uint16_t port = 0;
  auto [_, ec] = std::from_chars(port_string.data(),
                                 port_string.data() + port_string.size(), port);
  if (ec != std::errc{}) {
    return util::error{util::error_code::parser_error, "failed to parse port"};
  }
  return v4_endpoint{std::get<v4_address>(maybe_addr), port};
}

sockaddr_in to_sockaddr_in(const v4_endpoint& ep) {
  sockaddr_in saddr;
  saddr.sin_addr.s_addr = ep.address().bits();
  saddr.sin_port = htons(ep.port());
  saddr.sin_family = AF_INET;
  return saddr;
}

} // namespace net::ip
