/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include "net/receive_policy.hpp"

namespace net {

struct transport_extension {
  /// Configures the amount to be read next
  virtual void configure_next_read(receive_policy policy) = 0;

  /// Returns a reference to the send_buffer
  virtual util::byte_buffer& write_buffer() = 0;

  /// Enqueues data to the transport extension
  virtual void enqueue(util::const_byte_span bytes) = 0;

  /// Called when an error occurs
  virtual void handle_error(util::error err) = 0;

  /// Registers the stack for write events
  virtual void register_writing() = 0;
};

} // namespace net
