/**
 *  @author    Jakob Otto
 *  @file      socket.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "net/sockets/socket_id.hpp"

#include <compare>
#include <cstddef>
#include <string>
#include <sys/socket.h>

namespace net::sockets {

/// @brief base class of the typed socket abstraction.
struct socket {
  constexpr socket() noexcept = default;

  constexpr socket(socket_id x) noexcept : id{x} {
    // nop
  }

  constexpr std::strong_ordering operator<=>(const socket& other) const noexcept
    = default;

  /// @brief Contained `socket_id` of the socket
  socket_id id{invalid_socket_id};
};

/// @brief Denotes an invalid socket.
static constexpr socket invalid_socket{invalid_socket_id};

/// @brief Explicitly converts between different socket types
/// @param x  The socket that shall be converted
/// @returns socket of type To
template <class To, class From>
static constexpr To socket_cast(From x) noexcept {
  return To{x.id};
}

/// @brief Closes socket `x`.
/// @param[in] x  The socket so close
void close(socket x) noexcept;

/// @brief Enumeration of all possible channels that can be shutdown
enum class connection_channel : std::uint8_t {
  read       = SHUT_RD,
  write      = SHUT_WR,
  read_write = SHUT_RDWR,
};

/// @brief Shuts down socket `x`.
/// @param[in] x  The socket so shutdown
/// @param[in] how  The connection channel(s) to shutdown
void shutdown(socket x, connection_channel how) noexcept;

/// @brief Binds socket `x` to endpoint `ep`.
/// @param[in] x  The socket that should be bound
/// @param[in] ep  The endpoint to bind the socket to
/// @returns util::error in case the socket could not be bound
util::error bind(socket x, ip::v4_endpoint ep) noexcept;

/// @brief Returns the last socket error in this thread as an integer.
/// @returns an int denoting the last socket error (i.e. errno)
int last_socket_error() noexcept;

/// @brief Checks whether `last_socket_error()` would return an error code that
///        indicates a temporary error.
/// @returns `true` if the last socket error is temporary, false otherwise
bool last_socket_error_is_temporary() noexcept;

/// @brief Returns the last socket error as human-readable string.
/// @returns std::string_view containing a human readable error string
std::string last_socket_error_as_string() noexcept;

/// @brief Enables or disables nonblocking I/O on `x`.
/// @param[in] x  The socket that should be set to nonblocking
/// @param[in] enable  Whether to enable nonblocking
bool nonblocking(socket x, bool enable) noexcept;

/// @brief Returns port of `x` or an error if `x` is not bound.
/// @param[in] x the socket to retrieve the port of
/// @returns the port of the socket or util::error in case `x` is not bound
util::error_or<uint16_t> port_of(socket x) noexcept;

/// @brief Enables or disables reuseaddr on `x`.
/// @param[in] x  The socket to set reuseaddr on
/// @param[in] enable  Whether to enable reuseaddr
bool reuseaddr(socket x, bool enable) noexcept;

std::size_t mtu(socket handle) noexcept;

} // namespace net::sockets
