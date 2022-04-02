/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/ip/v4_endpoint.hpp"

#include "net/error.hpp"

#include "util/format.hpp"

namespace net::ip {

bool operator==(const v4_endpoint& lhs, const v4_endpoint& rhs) {
  return (lhs.address() == rhs.address()) && (lhs.port() == rhs.port());
}

bool operator!=(const v4_endpoint& lhs, const v4_endpoint& rhs) {
  return !(lhs == rhs);
}

std::string to_string(const v4_endpoint& ep) {
  return util::format("{0}:{1}", to_string(ep.address()), ep.port());
}

error_or<v4_endpoint> parse_v4_endpoint(std::string str) {
  const auto parts = util::split(std::move(str), ':');
  if (parts.size() != 2)
    return error{error_code::parser_error,
                 "Parsing to v4_endpoint failed: needs address and port, "
                 "separated by ':'"};
  auto maybe_addr = parse_v4_address(parts.front());
  if (auto err = get_error(maybe_addr))
    return *err;
  auto port = static_cast<std::uint16_t>(std::stoi(parts.back()));
  return v4_endpoint{std::move(std::get<v4_address>(maybe_addr)), port};
}

} // namespace net::ip
