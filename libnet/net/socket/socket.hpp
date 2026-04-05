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

#include "net/socket/socket_id.hpp"

#include <cstddef>
#include <string>

namespace net {

/// @brief Base class of the typed socket abstraction.
/// Represents a network socket with a unique identifier.
struct socket {
  /// @brief Default constructs a socket object with an invalid socket ID.
  constexpr socket() noexcept = default;

  /// @brief Constructs a socket object from a socket ID.
  /// @param id The socket ID.
  constexpr explicit socket(socket_id id) noexcept : id(id) {
    // nop
  }

  /// @brief Destructs
  constexpr ~socket() noexcept = default;

  /// @brief Copy constructor.
  constexpr socket(const socket& other) noexcept = default;

  /// @brief Copy assignment operator.
  constexpr socket& operator=(const socket& other) noexcept = default;

  /// @brief Comparison operator for equality of two sockets.
  /// @param other The socket to compare with.
  /// @return true if both sockets have the same ID, false otherwise.
  constexpr bool operator==(const socket other) const noexcept {
    return id == other.id;
  }

  /// @brief Comparison operator for inequality of two sockets.
  /// @param other The socket to compare with.
  /// @return true if the sockets have different IDs, false otherwise.
  constexpr bool operator!=(const socket other) const noexcept {
    return id != other.id;
  }

  /// @brief The contained socket ID.
  socket_id id{invalid_socket_id};
};

/// @brief Represents an invalid/uninitialized socket.
constexpr socket invalid_socket{invalid_socket_id};

/// @brief Converts between different socket types.
/// Performs a bitwise cast of the socket ID to the target type.
/// @tparam To The target socket type.
/// @tparam From The source socket type.
/// @param x The source socket to convert.
/// @return A socket of type `To` with the same ID as `x`.
template <class To, class From>
constexpr To socket_cast(From x) {
  return To{x.id};
}

/// @brief Closes the specified socket.
/// Releases the socket resources and marks it as invalid.
/// @param x The socket to close.
void close(socket x);

/// @brief Shuts down the specified socket.
/// Disables further communication on the socket.
/// @param sock The socket to shutdown.
/// @param how How to shutdown (SHUT_RD, SHUT_WR, or SHUT_RDWR).
void shutdown(socket sock, int how);

/// @brief Binds a socket to the specified endpoint.
/// @param sock The socket to bind.
/// @param ep The endpoint address and port to bind to.
/// @return An error if binding failed, util::none on success.
util::error bind(socket sock, ip::v4_endpoint ep);

/// @brief Returns the last socket error code in this thread.
/// @return The errno value from the last failed socket operation.
int last_socket_error();

/// @brief Checks whether the last socket error is temporary.
/// Determines if the error indicates a temporary condition that may succeed on
/// retry.
/// @return true if the error is temporary, false otherwise.
bool last_socket_error_is_temporary();

/// @brief Returns the last socket error as a human-readable string.
/// @return A descriptive error message for the last socket error.
std::string last_socket_error_as_string();

/// @brief Enables or disables nonblocking mode on a socket.
/// @param x The socket to modify.
/// @param new_value true to enable nonblocking mode, false to disable.
/// @return true if the operation succeeded, false otherwise.
bool nonblocking(socket x, bool new_value);

/// @brief Returns the port number of the specified socket.
/// @param x The socket to query.
/// @return The port number if the socket is bound, or an error if not.
util::error_or<uint16_t> port_of(socket x);

/// @brief Enables or disables the SO_REUSEADDR option on a socket.
/// Allows reusing a port that is in TIME_WAIT state.
/// @param x The socket to modify.
/// @param new_value true to enable SO_REUSEADDR, false to disable.
/// @return true if the operation succeeded, false otherwise.
bool reuseaddr(socket x, bool new_value);

} // namespace net
