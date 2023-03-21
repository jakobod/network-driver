/**
 *  @author    Jakob Otto
 *  @file      socket_id.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

namespace net {

/// type of the socket ids
using socket_id = int;

/// Denotes the invalid socket id
static constexpr const socket_id invalid_socket_id = -1;

} // namespace net
