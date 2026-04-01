/**
 *  @author    Jakob Otto
 *  @file      manager_factory.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "net/manager_base.hpp"
#include "net/socket/socket.hpp"

#include <functional>

namespace net {

using manager_factory
  = std::function<manager_base_ptr(net::socket, multiplexer_base*)>;

}
