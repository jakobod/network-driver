/**
 *  @author    Jakob Otto
 *  @file      transport.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/socket_manager.hpp"

#include "net/fwd.hpp"

#include "util/byte_buffer.hpp"
#include "util/byte_span.hpp"
#include "util/error.hpp"
#include "util/logger.hpp"

namespace net {

/// Implements a generic transport for use as pointer in the stack.
class transport : public socket_manager {
public:
  transport(socket handle, multiplexer* mpx) : socket_manager{handle, mpx} {
    // nop
  }

  util::error init(const util::config& cfg) override {
    LOG_TRACE();
    max_consecutive_fetches_ = cfg.get_or("transport.max-consecutive-fetches",
                                          std::int64_t{10});
    max_consecutive_reads_ = cfg.get_or("transport.max-consecutive-reads",
                                        std::int64_t{20});
    max_consecutive_writes_ = cfg.get_or("transport.max-consecutive-writes",
                                         std::int64_t{20});
    LOG_DEBUG(NET_ARG(max_consecutive_fetches_),
              NET_ARG(max_consecutive_reads_),
              NET_ARG(max_consecutive_writes_));
    return util::none;
  }

  // -- public API -------------------------------------------------------------

  /// Configures the amount to be read next
  virtual void configure_next_read(receive_policy policy) = 0;

  /// Returns a reference to the send_buffer
  util::byte_buffer& write_buffer() { return write_buffer_; }

  void enqueue(util::const_byte_span bytes) {
    LOG_TRACE();
    // TODO: copying the data is not very performant. Think of another solution
    write_buffer_.insert(write_buffer_.end(), bytes.begin(), bytes.end());
  }

protected:
  size_t max_consecutive_fetches_ = 10;
  size_t max_consecutive_reads_ = 20;
  size_t max_consecutive_writes_ = 20;

  size_t received_ = 0;
  size_t written_ = 0;
  size_t min_read_size_ = 0;

  util::byte_buffer read_buffer_;
  util::byte_buffer write_buffer_;
};

} // namespace net
