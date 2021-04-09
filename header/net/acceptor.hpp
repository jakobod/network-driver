/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 30.03.2021
 */

#pragma once

#include "net/socket_manager.hpp"

#include "fwd.hpp"
#include "net/tcp_accept_socket.hpp"

namespace net {

/// Manages the lifetime of a socket.
class acceptor : public socket_manager {
public:
  acceptor(tcp_accept_socket handle, multiplexer* mpx,
           socket_manager_factory_ptr factory);

  // -- properties -------------------------------------------------------------

  bool handle_read_event() override;

  bool handle_write_event() override;

private:
  socket_manager_factory_ptr factory_;
};

} // namespace net
