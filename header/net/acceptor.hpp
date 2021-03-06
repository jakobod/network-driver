/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/fwd.hpp"

#include "net/socket_manager.hpp"

#include "net/tcp_accept_socket.hpp"

namespace net {

/// Manages the lifetime of a socket.
class acceptor : public socket_manager {
public:
  acceptor(tcp_accept_socket handle, multiplexer* mpx,
           socket_manager_factory_ptr factory);

  ~acceptor() override = default;

  util::error init() override;

  // -- properties -------------------------------------------------------------

  event_result handle_read_event() override;

  event_result handle_write_event() override;

  event_result handle_timeout(uint64_t timeout_id) override;

private:
  socket_manager_factory_ptr factory_;
};

} // namespace net
