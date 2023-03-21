/**
 *  @author    Jakob Otto
 *  @file      layer.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "util/byte_buffer.hpp"

#include <chrono>

namespace net {

struct layer {
  // -- Upfacing interface (towards application) -------------------------------

  /// Initializes the application.
  virtual util::error init() = 0;

  /// Checks wether the application has more data to send.
  virtual bool has_more_data() = 0;

  /// Produces more data and enqueues it at their parent.
  virtual event_result produce() = 0;

  /// Consumes previously received data.
  virtual event_result consume(util::const_byte_span bytes) = 0;

  /// Handles a timeout.
  virtual event_result handle_timeout(uint64_t id) = 0;

  // -- Downfacing interface (towards transport) -------------------------------

  /// Configures the amount to be read next
  virtual void configure_next_read(receive_policy policy) = 0;

  /// Returns a reference to the send_buffer
  virtual util::byte_buffer& write_buffer() = 0;

  /// Enqueues data to the transport extension
  virtual void enqueue(util::const_byte_span bytes) = 0;

  /// Called when an error occurs
  virtual void handle_error(const util::error& err) = 0;

  /// Registers the stack for write events
  virtual void register_writing() = 0;

  /// Sets a timeout in `duration` milliseconds with the id `timeout_id`
  virtual uint64_t set_timeout_in(std::chrono::milliseconds duration) = 0;

  /// Sets a timeout at timepoint `point` with the id `timeout_id`
  virtual uint64_t set_timeout_at(std::chrono::system_clock::time_point point)
    = 0;
};

} // namespace net
