/**
 *  @author    Jakob Otto
 *  @file      uri.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "net/ip/v4_endpoint.hpp"

#include <span>
#include <string_view>
#include <vector>

namespace net {

/// @brief RFC-compliant Unified Resource Identifier (URI) implementation.
/// Parses and represents URIs with scheme, authority (endpoint), path,
/// query parameters, and fragments. Follows the standard URI syntax format.
class uri {
public:
  /// @brief Constructs a URI with all components.
  /// @param uri_string The string to construct the uri from
  explicit uri(std::string uri_string);

  /// @brief Retrieves the original uri string.
  /// @return The original URI string.
  std::string_view original() const noexcept { return original_; }

  /// @brief Returns the URI scheme.
  /// @return A const reference to the scheme string.
  std::string_view scheme() const noexcept { return scheme_; }

  /// @brief Returns the URI authority (endpoint).
  /// @return A const reference to the IPv4 endpoint.
  const ip::v4_endpoint& authority() const noexcept { return auth_; }

  /// @brief Returns the URI path components.
  /// @return A const reference to the path vector.
  std::span<const std::string_view> path() const noexcept { return path_; }

  /// @brief Returns the URI query parameters.
  /// @return A const reference to the queries vector.
  std::span<const std::string_view> queries() const noexcept {
    return queries_;
  }

  /// @brief Returns the URI fragment identifiers.
  /// @return A const reference to the fragments vector.
  std::span<const std::string_view> fragments() const noexcept {
    return fragments_;
  }

  friend bool operator==(const uri& lhs, const uri& rhs) noexcept = default;

  friend bool operator!=(const uri& lhs, const uri& rhs) noexcept = default;

private:
  std::string original_;

  // Parsed uri parts
  std::string_view scheme_;
  ip::v4_endpoint auth_;
  std::vector<std::string_view> path_;
  std::vector<std::string_view> queries_;
  std::vector<std::string_view> fragments_;
};

std::string_view to_string(const uri& uri) noexcept;

} // namespace net
