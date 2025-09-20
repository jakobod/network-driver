/**
 *  @author    Jakob Otto
 *  @file      socket_manager_factory.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "net/uring_manager.hpp"

namespace net {

/// Base class for socket manager factory classes.
class manager_factory {
public:
  virtual ~manager_factory() = default;

  virtual uring_manager_ptr
  make_uring_manager(sockets::socket handle, uring_multiplexer* mpx)
    = 0;
};

} // namespace net
