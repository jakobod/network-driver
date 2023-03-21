/**
 *  @author    Jakob Otto
 *  @file      socket_manager_factory.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

namespace net {

/// Base class for socket manager factory classes.
class socket_manager_factory {
public:
  virtual ~socket_manager_factory() = default;

  virtual socket_manager_ptr make(socket handle, multiplexer* mpx) = 0;
};

} // namespace net
