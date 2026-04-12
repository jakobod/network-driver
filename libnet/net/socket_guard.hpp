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

/// @brief RAII guard for automatic socket cleanup.
/// Manages ownership of a socket and ensures it is closed on scope exit,
/// preventing resource leaks. Supports move semantics but not copy semantics.
/// @tparam Socket A type derived from socket.
template <meta::derived_from<socket> Socket>
class socket_guard {
public:
  /// @brief Constructs a socket guard managing the specified socket.
  /// @param sock The socket to manage and close on destruction.
  explicit constexpr socket_guard(Socket sock) noexcept : sock_{sock} {
    // nop
  }

  /// @brief Default constructs an empty socket guard.
  /// The guard will not close anything on destruction.
  constexpr socket_guard() noexcept = default;

  /// @brief Copy construction is deleted (sockets cannot be shared).
  socket_guard(const socket_guard& other) = delete;

  /// @brief Move constructor transfers ownership.
  /// @param other The guard to move from.
  socket_guard(socket_guard&& other) noexcept : sock_{other.release()} {
    // nop
  }

  /// @brief Copy assignment is deleted (sockets cannot be shared).
  socket_guard& operator=(const socket_guard& other) = delete;

  socket_guard& operator=(Socket handle) {
    reset(handle);
    return *this;
  }

  /// @brief Move assignment transfers ownership and closes the previous socket.
  /// @param other The guard to move from.
  /// @return Reference to this.
  socket_guard& operator=(socket_guard&& other) noexcept {
    socket_guard tmp{std::move(other)};
    std::swap(*this, tmp);
    return *this;
  }

  /// @brief Destructor closes the managed socket on scope exit.
  /// The socket is only closed if it has been assigned (not invalid).
  ~socket_guard() { reset(); }

  /// @brief Releases ownership of the managed socket.
  /// Marks the socket as invalid in this guard, preventing close on
  /// destruction.
  /// @return The released socket object.
  Socket release() noexcept {
    const auto ret = sock_;
    sock_.id = invalid_socket_id;
    return ret;
  }

  void reset(Socket sock = Socket{invalid_socket_id}) noexcept {
    if (sock_ != invalid_socket) {
      close(sock_);
    }
    sock_ = sock;
  }

  /// @brief Returns the managed socket without releasing ownership.
  /// @return The socket object (still managed by this guard).
  Socket get() const noexcept { return sock_; }

  /// @brief Dereferences the managed socket (prefer get() for clarity).
  /// @return The socket object.
  Socket operator*() const noexcept { return sock_; }

  /// @brief Arrow operator for accessing socket members.
  /// @return A pointer to the managed socket.
  Socket* operator->() noexcept { return &sock_; }

  /// @brief Equality comparison between two guards.
  /// @param other The guard to compare with.
  /// @return True if both guards manage the same socket.
  bool operator==(const socket_guard<Socket>& other) const noexcept {
    return sock_ == other.sock_;
  }

private:
  /// @brief The managed socket object.
  Socket sock_{invalid_socket_id};
};

/// @brief Convenience factory function for creating a socket_guard.
/// Deduces the socket type automatically.
/// @tparam Socket A type derived from socket.
/// @param sock The socket to manage.
/// @return A socket_guard managing the socket.
template <meta::derived_from<socket> Socket>
constexpr socket_guard<Socket> make_socket_guard(Socket sock) {
  return socket_guard<Socket>{sock};
}

} // namespace net
