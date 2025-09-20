/**
 *  @author    Jakob Otto
 *  @file      socket_id.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

namespace net::sockets {

/// @brief Alias type for a socket_id
using socket_id = int;

/// @brief Denotes the invalid socket id
static constexpr socket_id invalid_socket_id = -1;

} // namespace net::sockets
