/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 29.03.2021
 */

#pragma once

#include "detail/byte_container.hpp"
#include "net/multiplexer.hpp"
#include "net/operation.hpp"
#include "net/socket.hpp"
#include "net/socket_manager.hpp"

namespace net {

class socket_manager_impl : public socket_manager {
public:
  // -- constructors -----------------------------------------------------------

  socket_manager_impl(socket handle, multiplexer* mpx, bool mirror);

  ~socket_manager_impl();

  // -- event handling ---------------------------------------------------------

  bool handle_read_event() override;

  bool handle_write_event() override;

private:
  bool mirror_;
  detail::byte_buffer write_buffer_;
  size_t received_bytes_;
  size_t sent_bytes_;
  size_t num_handled_events_;
};

} // namespace net
