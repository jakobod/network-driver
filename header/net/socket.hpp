/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *
 *  This file is based on `socket.hpp` from the C++ Actor Framework.
 *  https://github.com/actor-framework/incubator
 */

#pragma once

#include "fwd.hpp"

#include "net/error.hpp"
#include "net/socket_id.hpp"

#include "net/ip/v4_endpoint.hpp"

#include <net/if.h>
#include <string>

namespace net {

struct socket {
  socket_id id = invalid_socket_id;

  constexpr socket() noexcept : id(invalid_socket_id) {
    // nop
  }

  constexpr explicit socket(socket_id id) noexcept : id(id) {
    // nop
  }

  constexpr socket(const socket& other) noexcept = default;

  socket& operator=(const socket& other) noexcept = default;

  bool operator==(const socket other) const {
    return id == other.id;
  }

  bool operator!=(const socket other) const {
    return id != other.id;
  }
};

/// Denotes the invalid socket.
constexpr auto invalid_socket = socket{invalid_socket_id};

/// Converts between different socket types.
template <class To, class From>
To socket_cast(From x) {
  return To{x.id};
}

/// Closes socket `x`.
void close(socket x);

/// Shuts down socket `x`.
void shutdown(socket sock, int how);

/// binds socket `x` to endpoint `ep`.
error bind(socket sock, ip::v4_endpoint ep);

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
error_or<uint16_t> port_of(socket x);

/// Enables or disables reuseaddr I/O on `x`.
bool reuseaddr(socket x, bool new_value);

/// Returns the mac address of the specified interface.
ifreq get_if_mac(socket sock, const std::string& if_name);

/// Returns the index of the specified interface.
ifreq get_if_index(socket sock, const std::string& if_name);

} // namespace net
