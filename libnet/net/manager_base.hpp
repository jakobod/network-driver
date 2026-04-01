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

#include "net/operation.hpp"
#include "net/socket/socket.hpp"

#include "util/intrusive_ptr.hpp"
#include "util/ref_counted.hpp"

namespace net {

/// Manages the lifetime of a socket and its events.
class manager_base : public util::ref_counted {
public:
  /// Constructs a manager object
  manager_base(socket handle, multiplexer_base* mpx);

  /// Initializes a manager_base
  virtual util::error init(const util::config& cfg);

  /// Destructs a socket manager_base object
  virtual ~manager_base();

  // -- Move and assignment operations -----------------------------------------

  /// Deleted copy constructor
  manager_base(const manager_base&) = delete;

  /// Move constructor
  manager_base(manager_base&& mgr) noexcept;

  /// Deleted copy assignment operator
  manager_base& operator=(const manager_base&) = delete;

  /// Move assignment operator
  manager_base& operator=(manager_base&& other) noexcept;

  // -- properties -------------------------------------------------------------

  /// Returns the handle.
  template <class Socket = socket>
  Socket handle() const noexcept {
    return socket_cast<Socket>(handle_);
  }

  /// Returns a ptr to the multiplexer
  template <class Multiplexer = multiplexer_base>
  Multiplexer* mpx() const noexcept {
    return static_cast<Multiplexer*>(mpx_);
  }

  /// Returns registered operations (read, write, or both).
  operation mask() const noexcept { return mask_; }

  /// Sets the event mask to the given flag(s).
  void mask_set(operation flag) noexcept { mask_ = flag; }

  /// Adds given flag(s) to the event mask.
  bool mask_add(operation flag) noexcept;

  /// Tries to clear given flag(s) from the event mask.
  bool mask_del(operation flag) noexcept;

  // -- Event handling ---------------------------------------------------------

  void register_reading();

  void register_writing();

  virtual event_result handle_read_event();

  virtual event_result handle_write_event();

  // -- Timeout handling -------------------------------------------------------

  uint64_t set_timeout_in(std::chrono::steady_clock::duration in);

  uint64_t set_timeout_at(std::chrono::steady_clock::time_point when);

  virtual event_result handle_timeout(uint64_t);

private:
  /// The managed socket
  socket handle_{invalid_socket};
  /// Pointer to the multiplexer owning this manager_base
  multiplexer_base* mpx_{nullptr};
  /// The mask containing all currently registered events
  operation mask_{operation::none};
};

using manager_base_ptr = util::intrusive_ptr<manager_base>;

} // namespace net
