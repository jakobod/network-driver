/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/fwd.hpp"

#include "net/event_result.hpp"
#include "net/operation.hpp"
#include "net/socket.hpp"

#include <chrono>

namespace net {

/// Manages the lifetime of a socket and its events.
class socket_manager {
public:
  /// Constructs a socket_manager object
  socket_manager(socket handle, multiplexer* mpx);

  /// Destructs a socket manager object
  virtual ~socket_manager();

  /// Move constructor
  socket_manager(socket_manager&& mgr) noexcept;

  /// Deleted copy constructor
  socket_manager(const socket_manager&) = delete;

  /// Initializes a socket_manager
  virtual util::error init() = 0;

  // -- Assignment operators ---------------------------------------------------

  /// Move assignment operator
  socket_manager& operator=(socket_manager&& other) noexcept;

  /// Deleted copy assignment operator
  socket_manager& operator=(const socket_manager&) = delete;

  // -- properties -------------------------------------------------------------

  /// Returns a ptr to the multiplexer
  multiplexer* mpx() const noexcept {
    return mpx_;
  }

  /// Returns the handle.
  template <class Socket = socket>
  Socket handle() const noexcept {
    return socket_cast<Socket>(handle_);
  }

  /// Returns registered operations (read, write, or both).
  operation mask() const noexcept {
    return mask_;
  }

  /// Adds given flag(s) to the event mask.
  bool mask_add(operation flag) noexcept;

  /// Tries to clear given flag(s) from the event mask.
  bool mask_del(operation flag) noexcept;

  // -- event loop management --------------------------------------------------

  /// Registers this socket_manager for read-events at the multiplexer
  void register_reading();

  /// Registers this socket_manager for write-events at the multiplexer
  void register_writing();

  /// Sets a timeout in `duration` milliseconds with the id `timeout_id`
  void set_timeout_in(std::chrono::milliseconds duration, uint64_t timeout_id);

  /// Sets a timeout at timepoint `point` with the id `timeout_id`
  void set_timeout_at(std::chrono::system_clock::time_point point,
                      uint64_t timeout_id);

  // -- event handling ---------------------------------------------------------

  /// Handles a read-event
  virtual event_result handle_read_event() = 0;

  /// Handles a write-event
  virtual event_result handle_write_event() = 0;

  /// Handles a timeout-event
  virtual event_result handle_timeout(uint64_t timeout_id) = 0;

  /// Handles an error
  void handle_error(const util::error& err);

private:
  /// The managed socket
  socket handle_;
  /// Pointer to the multiplexer owning this socket-manager
  multiplexer* mpx_;
  /// The mask containing all currently registered events
  operation mask_;
};

} // namespace net
