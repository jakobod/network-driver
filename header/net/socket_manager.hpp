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
#include "net/event_result.hpp"
#include "net/operation.hpp"
#include "net/socket.hpp"

namespace net {

/// Manages the lifetime of a socket.
class socket_manager {
public:
  socket_manager(socket handle, multiplexer* parent);

  virtual ~socket_manager();

  socket_manager(socket_manager&& mgr);

  socket_manager(const socket_manager&) = delete;

  virtual error init() = 0;

  // -- Assignment operators ---------------------------------------------------

  socket_manager& operator=(socket_manager&& other);

  socket_manager& operator=(const socket_manager&) = delete;

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

  void set_timeout_in(std::chrono::milliseconds duration, uint64_t timeout_id);

  void set_timeout_at(std::chrono::system_clock::time_point point,
                      uint64_t timeout_id);

  // -- event handling ---------------------------------------------------------

  virtual event_result handle_read_event() = 0;

  virtual event_result handle_write_event() = 0;

  virtual event_result handle_timeout(uint64_t timeout_id) = 0;

  // -- result handling --------------------------------------------------------

  virtual socket_manager_ptr handle_accept(tcp_stream_socket handle) {
    mpx()->handle_error(error{runtime_error, "handle_accept not implemented"});
    return nullptr;
  };

  virtual event_result handle_data(util::const_byte_span data) {
    mpx()->handle_error(error{runtime_error, "handle_data not implemented"});
    return event_result::error;
  };

  virtual event_result write_data(util::const_byte_span data) {
    mpx()->handle_error(error{runtime_error, "write_data not implemented"});
    return event_result::error;
  }

  void handle_error(error err);

private:
  socket handle_;

  operation mask_;

  multiplexer* mpx_;
};

} // namespace net
