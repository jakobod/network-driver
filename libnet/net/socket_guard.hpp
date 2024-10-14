/**
 *  @author    Jakob Otto
 *  @file      socket_guard.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/socket/socket.hpp"

#include "meta/concepts.hpp"

namespace net {

template <meta::derived_from<socket> Socket>
class socket_guard {
public:
  /// Constructs a socket guard object from a given socket to manage.
  explicit constexpr socket_guard(Socket sock) noexcept : sock_{sock} {
    // nop
  }

  /// Default initializes a socket_guard object.
  constexpr socket_guard() noexcept = default;

  /// closes a managed socket object on destruction.
  ~socket_guard() {
    if (sock_ != invalid_socket) {
      close(sock_);
    }
  }

  /// Releases a managed socket object and gives up the ownership to the caller.
  Socket release() noexcept {
    const auto ret = sock_;
    sock_.id = invalid_socket_id;
    return ret;
  }

  /// Returns the managed socket object without giving up ownership.
  Socket get() const noexcept { return sock_; }

  /// Returns the managed socket object without giving up ownership.
  Socket operator*() const noexcept { return sock_; }

  /// Returns the managed socket object without giving up ownership.
  Socket* operator->() noexcept { return &sock_; }

  /// Compares two socket_guard objects for equality.
  bool operator==(const socket_guard<Socket>& other) const noexcept {
    return sock_ == other.sock_;
  }

private:
  /// The managed socket object.
  Socket sock_{invalid_socket};
};

/// Convenience function to create a socket_guard object without having to
/// specify template arguments.
template <meta::derived_from<socket> Socket>
constexpr socket_guard<Socket> make_socket_guard(Socket sock) {
  return socket_guard<Socket>{sock};
}

} // namespace net
