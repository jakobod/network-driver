/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 30.03.2021
 */

#pragma once

#include "net/socket_manager.hpp"

#include "net/fwd.hpp"

namespace net {

/// Manages the lifetime of a socket.
class acceptor : public socket_manager {
public:
  acceptor();

  // -- properties -------------------------------------------------------------

  bool handle_read_event(multiplexer* mpx);

  bool handle_write_event(multiplexer*);

private:
  multiplexer* mpx_;
};

} // namespace net
