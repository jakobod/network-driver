/**
 *  @author    Jakob Otto
 *  @file      manager.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "net/manager_base.hpp"

#include "net/operation.hpp"
#include "net/socket/socket.hpp"

#include "util/intrusive_ptr.hpp"

#include <chrono>
#include <functional>

namespace net::kqueue {

class multiplexer;

/// Manages the lifetime of a socket and its events.
class manager : public manager_base {
  friend class multiplexer;

public:
  /// Constructs a manager object
  manager(socket handle, multiplexer_base* mpx);

  /// Destructs a socket manager object
  virtual ~manager() = default;

  // -- event loop management --------------------------------------------------

  /// Registers this manager for read-events at the multiplexer
  void register_reading();

  /// Registers this manager for write-events at the multiplexer
  void register_writing();

  /// Sets a timeout in `duration` milliseconds with the id `timeout_id`
  uint64_t set_timeout_in(std::chrono::milliseconds duration);

  /// Sets a timeout at timepoint `point` with the id `timeout_id`
  uint64_t set_timeout_at(std::chrono::system_clock::time_point point);

  // -- event handling ---------------------------------------------------------

  /// Handles a read-event
  virtual event_result handle_read_event();

  /// Handles a write-event
  virtual event_result handle_write_event();

  /// Handles a timeout-event
  virtual event_result handle_timeout(uint64_t timeout_id);

  /// Handles an error
  void handle_error(const util::error& err);
};

using manager_ptr = util::intrusive_ptr<manager>;

} // namespace net::kqueue
