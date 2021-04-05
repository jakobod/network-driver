/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 29.03.2021
 *
 *  This file is based on `socket_manager.hpp` from the C++ Actor Framework.
 *  https://github.com/actor-framework/incubator
 */

#pragma once

#include "net/fwd.hpp"
#include "net/operation.hpp"
#include "net/socket.hpp"

namespace net {

/// Manages the lifetime of a socket.
class socket_manager {
public:
  socket_manager(socket handle, multiplexer* parent);

  virtual ~socket_manager();

  virtual void init() = 0;

  // -- properties -------------------------------------------------------------

  /// Returns a ptr to the multiplexer
  multiplexer* mpx() {
    return mpx_;
  }

  /// Returns the handle.
  socket handle() const noexcept {
    return handle_;
  }

  /// Returns registered operations (read, write, or both).
  operation mask() const noexcept {
    return mask_;
  }

  /// Adds given flag(s) to the event mask.
  /// @returns `false` if `mask() | flag == mask()`, `true` otherwise.
  /// @pre `flag != operation::none`
  bool mask_add(operation flag) noexcept;

  /// Tries to clear given flag(s) from the event mask.
  /// @returns `false` if `mask() & ~flag == mask()`, `true` otherwise.
  /// @pre `flag != operation::none`
  bool mask_del(operation flag) noexcept;

  // -- event loop management --------------------------------------------------

  void register_reading();

  void register_writing();

  // -- event handling ---------------------------------------------------------

  virtual bool handle_read_event() = 0;

  virtual bool handle_write_event() = 0;

private:
  socket handle_;

  operation mask_;

  multiplexer* mpx_;
};

} // namespace net
