/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 04.03.2021
 *
 *  This file is based on `socket.hpp` from the C++ Actor Framework.
 *  https://github.com/actor-framework/incubator
 */

#pragma once

#include <net/if.h>
#include <string>

#include "net/socket_id.hpp"

namespace net {

struct socket {
  socket_id id;

  constexpr socket() noexcept : id(invalid_socket_id) {
    // nop
  }

  constexpr explicit socket(socket_id id) noexcept : id(id) {
    // nop
  }

  constexpr socket(const socket& other) noexcept = default;

  socket& operator=(const socket& other) noexcept = default;

  bool operator==(const int other_id) const {
    return id == other_id;
  }

  bool operator!=(const int other_id) const {
    return id != other_id;
  }
};

/// Denotes the invalid socket.
constexpr auto invalid_socket = socket{invalid_socket_id};

/// Closes socket `x`.
void close(socket x);

/// Returns the last socket error in this thread as an integer.
int last_socket_error();

/// Checks whether `last_socket_error()` would return an error code that
/// indicates a temporary error.
bool last_socket_error_is_temporary();

/// Returns the last socket error as human-readable string.
std::string last_socket_error_as_string();

/// Enables or disables nonblocking I/O on `x`.
bool nonblocking(socket x, bool new_value);

/// Returns the mac address of the specified interface.
ifreq get_if_mac(socket sock, const std::string& if_name);

/// Returns the index of the specified interface.
ifreq get_if_index(socket sock, const std::string& if_name);

} // namespace net
