/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include "net/receive_policy.hpp"
#include "net/socket.hpp"
#include "net/socket_manager.hpp"

#include "util/error.hpp"

namespace net {

/// Implements a generic transport for use as pointer in the stack.
class transport : public socket_manager {
public:
  transport(socket handle, multiplexer* mpx) : socket_manager{handle, mpx} {
    // nop
  }

  // -- public API -------------------------------------------------------------

  /// Configures the amount to be read next
  virtual void configure_next_read(receive_policy policy) = 0;

  /// Returns a reference to the send_buffer
  util::byte_buffer& write_buffer() {
    return write_buffer_;
  }

  void enqueue(util::const_byte_span bytes) {
    write_buffer_.insert(write_buffer_.end(), bytes.begin(), bytes.end());
  }

protected:
  static constexpr const size_t max_consecutive_fetches = 10;
  static constexpr const size_t max_consecutive_reads = 20;
  static constexpr const size_t max_consecutive_writes = 20;

  size_t received_ = 0;
  size_t written_ = 0;
  size_t min_read_size_ = 0;

  util::byte_buffer read_buffer_;
  util::byte_buffer write_buffer_;
};

} // namespace net
