/**
 *  @author    Jakob Otto
 *  @file      manager_base.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "net/event_result.hpp"

#include "net/detail/manager_base.hpp"

#include "util/exception.hpp"
#include "util/logger.hpp"

namespace net::detail {

/// @brief Base class for socket event handlers.
/// Provides virtual methods for handling read and write events on sockets.
/// Subclasses override these methods to implement protocol-specific event
/// handling.
class event_handler : public manager_base {
public:
  /// @brief Constructs an event handler for the given socket.
  /// Sets the socket to non-blocking mode for event-driven I/O.
  /// @param handle The socket to manage.
  /// @param mpx The multiplexer that owns this handler.
  event_handler(net::socket handle, multiplexer_base* mpx)
    : manager_base{handle, mpx} {
    LOG_TRACE();
  }

  util::error init(const util::config& cfg) override {
    LOG_TRACE();
    if (auto err = manager_base::init(cfg)) {
      return err;
    }
    if (!nonblocking(handle(), true)) {
      return util::error{util::error_code::runtime_error,
                         "Failed to set nonblocking"};
    }
    return util::none;
  }

  // -- Event handling ---------------------------------------------------------

  /// @brief Handles a read event on the managed socket.
  /// Subclasses should override this to process incoming data.
  /// @return event_result::ok if handling succeeded, event_result::error on
  /// failure,
  ///         event_result::done if the handler is finished.
  virtual event_result handle_read_event() { return event_result::error; }

  /// @brief Handles a write event on the managed socket.
  /// Subclasses should override this to process outgoing data.
  /// @return event_result::ok if handling succeeded, event_result::error on
  /// failure,
  ///         event_result::done if the handler is finished.
  virtual event_result handle_write_event() { return event_result::error; }
};

/// @brief Shared pointer type for event handlers.
using event_handler_ptr = util::intrusive_ptr<event_handler>;

} // namespace net::detail
