/**
 *  @author    Jakob Otto
 *  @file      socket_guard.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/sockets/socket.hpp"

#include "meta/concepts.hpp"

namespace net::sockets {

template <meta::derived_from<socket> Socket>
class socket_guard {
public:
  /// @brief Constructs a socket guard object from a given `socket` to manage.
  /// @param sock  The socket to manage.
  explicit socket_guard(Socket sock) noexcept : sock_{sock} {
    // nop
  }

  /// @brief Default initializes a socket_guard object.
  constexpr socket_guard() noexcept = default;

  /// @brief closes a managed socket object on destruction.
  ~socket_guard() {
    if (sock_ != invalid_socket) {
      close(sock_);
    }
  }

  /// @brief Deleted copy constructor
  socket_guard(const socket_guard& other) noexcept = delete;

  /// @brief Deleted copy assignment operator
  socket_guard& operator=(const socket_guard& other) noexcept = delete;

  /// @brief Move constructor
  /// @param other  The instance to move from
  socket_guard(socket_guard&& other) noexcept : sock_{other.release()} {
    // nop
  }

  /// @brief Move assignment operator
  /// @param other  The instance to move from
  /// @returns this instance
  socket_guard& operator=(socket_guard&& other) noexcept {
    sock_ = other.release();
  }

  /// @brief Releases a managed socket object and gives up the ownership to the
  /// caller.
  /// @returns the socket that was held before releasing
  Socket release() noexcept {
    auto ret = sock_;
    sock_ = socket_cast<Socket>(invalid_socket);
    return ret;
  }

  /// @brief Returns the managed socket object without giving up ownership.
  /// @returns the managed socket
  Socket get() const noexcept { return sock_; }

  /// @brief Returns the managed socket object without giving up ownership.
  /// @returns the managed socket
  Socket operator*() const noexcept { return sock_; }

  /// @brief Returns the managed socket object without giving up ownership.
  /// @returns the managed socket
  Socket* operator->() noexcept { return &sock_; }

  /// @brief Compares two socket_guard objects for equality.
  /// @param other  The socket to compare with
  /// @returns the comparison result
  std::strong_ordering operator<=>(const socket_guard& other) const noexcept {
    return sock_ <=> other.sock_;
  }

private:
  /// @brief The managed socket object.
  Socket sock_{invalid_socket};
};

/// Convenience function to create a socket_guard object without having to
/// specify template arguments.
template <meta::derived_from<socket> Socket>
constexpr socket_guard<Socket> make_socket_guard(Socket sock) {
  return socket_guard<Socket>{sock};
}

} // namespace net::sockets
