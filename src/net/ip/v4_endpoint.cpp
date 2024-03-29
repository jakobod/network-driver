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
#include "util/format.hpp"

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
  return util::format("{0}:{1}", to_string(ep.address()), ep.port());
}

util::error_or<v4_endpoint> parse_v4_endpoint(const std::string& str) {
  const auto parts = util::split(str, ':');
  if (parts.size() != 2)
    return util::error{util::error_code::parser_error,
                       "Parsing to v4_endpoint failed: needs address and port, "
                       "separated by ':'"};
  auto maybe_addr = parse_v4_address(parts.front());
  if (auto err = util::get_error(maybe_addr))
    return *err;
  auto port = static_cast<std::uint16_t>(std::stoi(parts.back()));
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
