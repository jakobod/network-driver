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

#include <chrono>

namespace net::detail {

/// Manages the lifetime of a socket and its events.
class manager_base : public util::ref_counted {
public:
  // -- Constructors, destructors, initialization ------------------------------

  /// @brief Constructs a manager object
  /// @param handle  The handle to manage
  /// @param mpx  The owning multiplexer
  manager_base(socket handle, multiplexer_base* mpx);

  /// @brief Initializes the manager_base
  /// @param cfg
  /// @return
  virtual util::error init(const util::config& cfg);

  /// @brief Destructs a socket manager_base object
  virtual ~manager_base();

  // -- Move and assignment operations -----------------------------------------

  /// @brief Deleted copy construction
  manager_base(const manager_base&) = delete;
  /// @brief Deleted copy assignment
  manager_base& operator=(const manager_base&) = delete;
  /// @brief Deleted copy construction
  manager_base(manager_base&&) noexcept = delete;
  /// @brief Deleted move assignment
  manager_base& operator=(manager_base&&) noexcept = delete;

  // -- properties -------------------------------------------------------------

  /// @brief Returns the operation for which this manager should be initially
  /// registered
  /// @returns operation denoting the initial operation to register for
  virtual operation initial_operation() const noexcept {
    return operation::read;
  }

  /// @brief Retrieves the managed handle
  /// @tparam Socket  The type to cast the handle to, net::socket by default
  /// @returns the managed handle
  template <class Socket = socket>
  Socket handle() const noexcept {
    return socket_cast<Socket>(handle_);
  }

  /// @brief Returns the owning multiplexer
  /// @tparam Multiplexer  The type to cast the multiplexer to,
  ///.                     detail::multiplexer_base by default
  /// @return a reference to the owning multiplexer
  template <class Multiplexer = detail::multiplexer_base>
  Multiplexer* mpx() const noexcept {
    return static_cast<Multiplexer*>(mpx_);
  }

  /// @brief Returns registered operation mask.
  /// @returns operation mask denoting the currently registered operations
  operation mask() const noexcept { return mask_; }

  /// @brief Sets the event mask to the given flag(s).
  /// @param flag The operation flag(s) to set as the mask
  void mask_set(operation flag) noexcept { mask_ = flag; }

  /// @brief Adds given flag(s) to the event mask.
  /// @param flag The operation flag(s) to add
  /// @return true if the mask was modified, false otherwise
  bool mask_add(operation flag) noexcept;

  /// @brief Tries to clear given flag(s) from the event mask.
  /// @param flag The operation flag(s) to remove
  /// @return true if the mask was modified, false otherwise
  bool mask_del(operation flag) noexcept;

  // -- Event handling ---------------------------------------------------------

  /// @brief Registers this manager for read events on its socket.
  void register_reading();

  /// @brief Registers this manager for write events on its socket.
  void register_writing();

  // -- Timeout handling -------------------------------------------------------

  /// @brief Sets a timeout to trigger after the specified duration.
  /// @param in The duration to wait before the timeout fires
  /// @return A unique timeout identifier that can be used to cancel or identify
  /// the timeout
  uint64_t set_timeout_in(std::chrono::steady_clock::duration in);

  /// @brief Sets a timeout to trigger at the specified absolute time point.
  /// @param when The absolute time point at which the timeout should fire
  /// @return A unique timeout identifier that can be used to cancel or identify
  ///         the timeout
  uint64_t set_timeout_at(std::chrono::steady_clock::time_point when);

  /// @brief Handles a timeout event for this manager.
  /// @param timeout_id The identifier of the timeout that fired
  /// @return The result of handling the timeout event
  virtual event_result handle_timeout(uint64_t timeout_id);

private:
  /// The managed socket handle
  socket handle_{invalid_socket};
  /// Reference to the multiplexer owning this manager_base
  multiplexer_base* mpx_{nullptr};
  /// The mask containing all currently registered events
  operation mask_{operation::none};
};

/// @brief Alias for util::intrusive_ptr<manager_base>
using manager_base_ptr = util::intrusive_ptr<manager_base>;

} // namespace net::detail
