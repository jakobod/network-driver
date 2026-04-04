/**
 *  @author    Jakob Otto
 *  @file      socket_id.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

namespace net {

/// @brief Type alias for socket file descriptors.
/// Represents a system socket as a non-negative integer (file descriptor).
using socket_id = int;

/// @brief Constant representing an invalid or uninitialized socket.
/// Used to denote sockets that have not been successfully created.
static constexpr socket_id invalid_socket_id = -1;

} // namespace net
