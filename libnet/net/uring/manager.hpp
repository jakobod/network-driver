/**
 *  @author    Jakob Otto
 *  @file      uring_manager.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "net/operation.hpp"
#include "net/sockets/socket.hpp"

#include "util/intrusive_ptr.hpp"
#include "util/ref_counted.hpp"

#include <chrono>

struct io_uring_cqe;

namespace net::uring {

/// Manages the lifetime of a socket and its events.
class manager : public util::ref_counted {
public:
  /// Constructs a uring_manager object
  manager(sockets::socket handle, multiplexer_impl* mpx);

  /// Destructs a socket manager object
  ~manager() override;

  /// Initializes a uring_manager
  virtual util::error init(const util::config& cfg) = 0;

  // -- properties -------------------------------------------------------------

  /// Returns a ptr to the multiplexer
  multiplexer_impl* mpx() const noexcept { return mpx_; }

  /// Returns the handle.
  template <class Socket = sockets::socket>
  constexpr Socket handle() const noexcept {
    return socket_cast<Socket>(handle_);
  }

  /// Returns registered operations (read, write, or both).
  operation mask() const noexcept { return mask_; }

  /// Sets the event mask to the given flag(s).
  void mask_set(operation flag) noexcept;

  /// Adds given flag(s) to the event mask.
  bool mask_add(operation flag) noexcept;

  /// Tries to clear given flag(s) from the event mask.
  bool mask_del(operation flag) noexcept;

  // -- event handling ---------------------------------------------------------

  /// Handles a read-event
  virtual event_result handle_read_completion(const io_uring_cqe& cqe) = 0;

  /// Handles a write-event
  virtual event_result handle_write_completion(const io_uring_cqe& cqe) = 0;

private:
  /// The managed socket
  sockets::socket handle_;
  /// Pointer to the uring_multiplexer owning this socket-manager
  multiplexer_impl* mpx_;
  /// The mask containing all currently registered events
  operation mask_;
};

} // namespace net::uring
