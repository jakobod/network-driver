/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 29.03.2021
 *
 *  This file is based on `socket_manager.hpp` from the C++ Actor Framework.
 *  https://github.com/actor-framework/incubator
 */

#pragma once

#include "fwd.hpp"
#include "net/operation.hpp"
#include "net/socket.hpp"

namespace net {

/// Manages the lifetime of a socket.
class socket_manager {
public:
  socket_manager(socket handle, multiplexer* parent);

  virtual ~socket_manager();

  // -- properties -------------------------------------------------------------

  /// Returns a ptr to the multiplexer
  multiplexer* mpx() {
    return mpx_;
  }

  /// Returns the handle.
  /// @param Socket
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

  void register_reading();

  void register_writing();

  // -- event handling ---------------------------------------------------------

  virtual bool handle_read_event() = 0;

  virtual bool handle_write_event() = 0;

  // -- stringification --------------------------------------------------------

  virtual std::string me() const {
    return "socket_manager";
  }

  virtual std::string additional_info() const {
    return "";
  }

  friend std::ostream& operator<<(std::ostream& os, const socket_manager& mgr) {
    return os << "[" << mgr.me() << "]: handle = " << mgr.handle().id
              << ", mask = " << to_string(mgr.mask()) << ", "
              << mgr.additional_info();
  }

private:
  socket handle_;

  operation mask_;

  multiplexer* mpx_;
};

} // namespace net
