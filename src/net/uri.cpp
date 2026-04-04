/**
 *  @author    Jakob Otto
 *  @file      uri.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/uri.hpp"

#include "util/error.hpp"
#include "util/error_or.hpp"
#include "util/exception.hpp"
#include "util/format.hpp"

#include <ranges>
#include <sstream>

namespace {

std::string_view extract_parts(std::vector<std::string_view>& dst,
                               std::string_view source, auto delim) noexcept {
  dst = util::split(source, delim);
  const auto remainder = std::move(dst.front());
  dst.erase(dst.begin());
  return remainder;
}

} // namespace

namespace net {

uri::uri(std::string uri_string) : original_{std::move(uri_string)} {
  // Get the scheme of the uri removing the slashes
  auto parts = util::split(original_, "://");
  if (parts.size() > 2) {
    throw util::exception{util::error_code::parser_error,
                          util::format("Failed to parse {0}", original_)};
  }

  scheme_ = parts.front();
  const auto fragments_remainder = extract_parts(fragments_, parts.back(), '#');
  const auto queries_remainder = extract_parts(queries_, fragments_remainder,
                                               '?');
  const auto auth = extract_parts(path_, queries_remainder, '/');

  // Parse the authority
  auto maybe_auth = net::ip::parse_v4_endpoint(auth);
  if (auto err = util::get_error(maybe_auth)) {
    throw util::exception{std::move(*err)};
  }
  auth_ = std::move(std::get<ip::v4_endpoint>(maybe_auth));
}

bool operator==(const uri& lhs, const uri& rhs) {
  return (lhs.scheme() == rhs.scheme()) && (lhs.authority() == rhs.authority())
         && std::ranges::equal(lhs.path(), rhs.path())
         && std::ranges::equal(lhs.queries(), rhs.queries())
         && std::ranges::equal(lhs.fragments(), rhs.fragments());
}

bool operator!=(const uri& lhs, const uri& rhs) {
  return !(lhs == rhs);
}

std::string_view to_string(const uri& uri) noexcept {
  return uri.original();
}

} // namespace net
