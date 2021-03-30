/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 29.03.2021
 *
 *  This file is based on `socket_manager.hpp` from the C++ Actor Framework.
 *  https://github.com/actor-framework/incubator
 */

#pragma once

#include "net/multiplexer.hpp"
#include "net/operation.hpp"
#include "net/socket.hpp"
#include "net/socket_manager.hpp"

namespace net {

class socket_manager_impl : public socket_manager {
public:
  // -- constructors -----------------------------------------------------------

  socket_manager_impl();

  // -- event handling ---------------------------------------------------------

  bool handle_read_event() override;

  bool handle_write_event() override;
};

} // namespace net
