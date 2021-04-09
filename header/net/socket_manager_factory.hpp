/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 07.04.2021
 */

#pragma once

#include "fwd.hpp"

namespace net {

/// Base class for socket manager factory classes.
class socket_manager_factory {
public:
  virtual socket_manager_ptr make(socket handle, multiplexer* mpx) = 0;
};

} // namespace net
