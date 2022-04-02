/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/ip/v4_address.hpp"
#include "net/error.hpp"

#include "util/format.hpp"

#include <iomanip>
#include <ranges>
#include <string_view>

namespace net::ip {

bool operator==(const v4_address& lhs, const v4_address& rhs) {
  return lhs.bits() == rhs.bits();
}

bool operator!=(const v4_address& lhs, const v4_address& rhs) {
  return !(lhs == rhs);
}

std::string to_string(const v4_address& addr) {
  return util::format("{0}.{1}.{2}.{3}",
                      static_cast<std::uint8_t>(addr.bytes()[0]),
                      static_cast<std::uint8_t>(addr.bytes()[1]),
                      static_cast<std::uint8_t>(addr.bytes()[2]),
                      static_cast<std::uint8_t>(addr.bytes()[3]));
}

error_or<v4_address> parse_v4_address(std::string str) {
  v4_address::byte_array bytes;
  const auto parts = util::split(std::move(str), '.');
  if (parts.size() != bytes.size())
    return error{error_code::parser_error,
                 "Parsing to v4_endpoint failed: needs exactly 4 octets"};
  auto it = bytes.begin();
  for (const auto& part : parts)
    *it++ = std::byte{std::stoi(part)};
  return {bytes};
}

} // namespace net::ip
