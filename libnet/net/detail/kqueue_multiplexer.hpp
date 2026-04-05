/**
 *  @author    Jakob Otto
 *  @file      detail/kqueue_multiplexer.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#if !defined(__APPLE__)
#  error "kqueue_multiplexer is only usable on MacOS"
#else

#  include "net/fwd.hpp"
#  include "util/fwd.hpp"

#  include "net/detail/acceptor.hpp"
#  include "net/detail/event_handler.hpp"

#  include "net/detail/multiplexer_base.hpp"

#  include <array>
#  include <cstdint>
#  include <span>
#  include <vector>

#  include <sys/event.h>

namespace net::detail {

/// @brief Event multiplexer using macOS kqueue backend.
/// Implements the multiplexer interface for macOS systems using the kqueue
/// mechanism for scalable I/O event handling. Manages sockets and coordinates
/// with event handlers for read/write operations. Only available on macOS.
class kqueue_multiplexer : public multiplexer_base {
  /// @brief Maximum number of events returned by a single poll.
  static constexpr std::size_t max_events = 32;

public:
  /// @brief Factory function type for creating event handlers.
  using manager_factory
    = std::function<event_handler_ptr(net::socket, kqueue_multiplexer*)>;

private:
  using event_type = struct kevent;
  using mpx_fd = int;

  // Pollset types
  /// @brief Container for events returned from kqueue.
  using pollset = std::array<event_type, max_events>;
  /// @brief Pending kqueue modifications.
  using update_list = std::vector<event_type>;
  /// @brief View over event arrays.
  using event_span = std::span<event_type>;

public:
  // -- constructors, destructors ----------------------------------------------

  /// @brief Default constructs a kqueue multiplexer.
  kqueue_multiplexer() = default;

  /// @brief Destructs the kqueue multiplexer and closes the kqueue file
  /// descriptor.
  virtual ~kqueue_multiplexer();

  /// @brief Initializes the kqueue multiplexer with the given configuration.
  /// Creates a kqueue and sets up event monitoring.
  /// @param factory Factory function for creating event handlers.
  /// @param cfg Configuration parameters for the multiplexer.
  /// @return An error on failure, none on success.
  util::error init(manager_factory factory, const util::config& cfg);

  // -- Interface functions ---------------------------------------------------

  /// @brief Registers a socket manager for event monitoring.
  /// @param mgr The manager to register.
  /// @param initial The initial operations to monitor (read/write/accept).
  void add(manager_base_ptr mgr, operation initial) override;

  /// @brief Enables monitoring of an operation for a manager.
  /// @param mgr The manager whose operations to modify.
  /// @param op The operation to enable (read/write).
  void enable(manager_base& mgr, operation op) override;

  /// @brief Disables monitoring of an operation for a manager.
  /// Removes the manager entirely if no operations remain and remove is true.
  /// @param mgr The manager whose operations to modify.
  /// @param op The operation to disable.
  /// @param remove If true, delete the manager if no operations remain.
  void disable(manager_base& mgr, operation op, bool remove) override;

  /// @brief Performs a single kevent and processes all ready events.
  /// @param blocking If true, wait indefinitely for events; if false, return
  /// immediately.
  /// @return An error on failure, none on success.
  util::error poll_once(bool blocking) override;

private:
  /// @brief Dispatches all ready events to their associated handlers.
  /// @param events The events returned by kevent.
  void handle_events(event_span events);

  /// @brief Unregisters a socket manager from the kqueue.
  /// @param handle The socket identifier.
  void del(socket handle) override;

  /// @brief Unregisters a socket manager using an iterator.
  /// @param it Iterator to the manager in the manager map.
  /// @return Iterator to the element following the erased element.
  manager_map::iterator del(manager_map::iterator it) override;

  /// @brief Modifies kqueue monitoring settings for an existing file
  /// descriptor.
  /// @param fd The file descriptor to modify.
  /// @param op The kqueue operation code.
  /// @param events The operations to monitor (read/write).
  void mod(int fd, int op, operation events);

  // Multiplexing variables
  mpx_fd mpx_fd_{invalid_socket_id}; ///< The kqueue file descriptor
  pollset pollset_;                  ///< Buffer for events returned by kevent
  update_list update_cache_;         ///< Pending kqueue modifications
};

} // namespace net::detail

#endif
