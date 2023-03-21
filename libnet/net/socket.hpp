/**
 *  @author    Jakob Otto
 *  @file      socket.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "net/socket_id.hpp"

#include "util/fwd.hpp"

namespace net {

/// base class of the typed socket abstraction.
struct socket {
  /// Default constructs a socket object
  constexpr socket() noexcept : id(invalid_socket_id) {
    // nop
  }

  /// Constructs a socket object from a socekt_id
  constexpr explicit socket(socket_id id) noexcept : id(id) {
    // nop
  }

  /// Copy constructor
  constexpr socket(const socket& other) noexcept = default;

  /// Copy assignment operator
  socket& operator=(const socket& other) noexcept = default;

  /// Comparison operator for equality of two sockets
  bool operator==(const socket other) const { return id == other.id; }

  /// Comparison operator for inequality of two sockets
  bool operator!=(const socket other) const { return id != other.id; }

  /// contained socket_id of the socket
  socket_id id{invalid_socket_id};
};

/// Denotes the invalid socket.
constexpr socket invalid_socket{invalid_socket_id};

/// Converts between different socket types.
template <class To, class From>
constexpr To socket_cast(From x) {
  return To{x.id};
}

/// Closes socket `x`.
void close(socket x);

/// Shuts down socket `x`.
void shutdown(socket sock, int how);

/// binds socket `x` to endpoint `ep`.
util::error bind(socket sock, ip::v4_endpoint ep);

/// Returns the last socket error in this thread as an integer.
int last_socket_error();

/// Checks whether `last_socket_error()` would return an error code that
/// indicates a temporary error.
bool last_socket_error_is_temporary();

/// Returns the last socket error as human-readable string.
std::string last_socket_error_as_string();

/// Enables or disables nonblocking I/O on `x`.
bool nonblocking(socket x, bool new_value);

/// Returns port of `x` or error if `x` is not bound.
util::error_or<uint16_t> port_of(socket x);

/// Enables or disables reuseaddr I/O on `x`.
bool reuseaddr(socket x, bool new_value);

} // namespace net
