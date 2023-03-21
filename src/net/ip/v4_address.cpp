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
#include "util/format.hpp"

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

util::error_or<v4_address> parse_v4_address(const std::string& str) {
  v4_address::octet_array bytes;
  const auto parts = util::split(str, '.');
  if (parts.size() != bytes.size())
    return util::error{util::error_code::parser_error,
                       "Parsing to v4_endpoint failed: needs exactly 4 octets"};
  auto it = bytes.begin();
  for (const auto& part : parts)
    *it++ = std::byte{static_cast<std::uint8_t>(std::stoi(part))};
  return {bytes};
}

} // namespace net::ip
