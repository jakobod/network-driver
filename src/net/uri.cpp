/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/uri.hpp"

#include "util/format.hpp"

#include <sstream>

namespace net {

bool operator==(const uri& lhs, const uri& rhs) {
  return (lhs.scheme() == rhs.scheme()) && (lhs.authority() == rhs.authority())
         && (lhs.path() == rhs.path()) && (lhs.queries() == rhs.queries())
         && (lhs.fragments() == rhs.fragments());
}

bool operator!=(const uri& lhs, const uri& rhs) {
  return !(lhs == rhs);
}

std::string to_string(const uri& x) {
  std::stringstream ss;
  ss << x.scheme() << "://" << to_string(x.authority());
  for (const auto& p : x.path())
    ss << "/" << p;
  for (const auto& q : x.queries())
    ss << "?" << q;
  for (const auto& f : x.fragments())
    ss << "#" << f;
  return ss.str();
}

util::error_or<uri> parse_uri(std::string str) {
  // Get the scheme of the uri removing the slashes
  auto parts = util::split(str, "://");
  if (parts.size() > 2)
    return util::error{util::error_code::parser_error,
                       util::format("Failed to parse {0}", str)};
  auto scheme = parts.front();
  str = parts.back();

  // Get fragements (if exist)
  auto fragments = util::split(str, '#');
  str = fragments.front();
  fragments.erase(fragments.begin());

  auto queries = util::split(str, '?');
  str = queries.front();
  queries.erase(queries.begin());

  // Get the paths
  auto path = util::split(str, '/');
  auto auth_str = path.front();
  path.erase(path.begin());

  // Parse the authority
  auto maybe_auth = net::ip::parse_v4_endpoint(auth_str);
  if (auto err = util::get_error(maybe_auth))
    return *err;

  return uri{std::move(scheme), std::get<net::ip::v4_endpoint>(maybe_auth),
             std::move(path), std::move(queries), std::move(fragments)};
}

} // namespace net
