/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/ip/v4_endpoint.hpp"

#include "util/error_or.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace net {

/// RFC compliant Unified Ressource Identifier implementation.
class uri {
public:
  uri(std::string scheme, const ip::v4_endpoint& auth,
      std::vector<std::string> path, std::vector<std::string> queries,
      std::vector<std::string> fragments)
    : scheme_{std::move(scheme)},
      auth_{auth},
      path_{std::move(path)},
      queries_{std::move(queries)},
      fragments_{std::move(fragments)} {
    // nop
  }

  const std::string& scheme() const {
    return scheme_;
  }

  const ip::v4_endpoint& authority() const {
    return auth_;
  }

  const std::vector<std::string>& path() const {
    return path_;
  }

  const std::vector<std::string>& queries() const {
    return queries_;
  }

  const std::vector<std::string>& fragments() const {
    return fragments_;
  }

private:
  std::string scheme_;
  ip::v4_endpoint auth_;
  std::vector<std::string> path_;
  std::vector<std::string> queries_;
  std::vector<std::string> fragments_;
};

/// Compares two uri for equality.
bool operator==(const uri& lhs, const uri& rhs);

/// Compares two uri for inequality.
bool operator!=(const uri& lhs, const uri& rhs);

/// Returns a uri as string.
std::string to_string(const uri& addr);

/// parses a uri from string.
util::error_or<uri> parse_uri(std::string str);

} // namespace net
