/**
 *  @author    Jakob Otto
 *  @file      detail/epoll_multiplexer_impl.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#if !defined(__linux__)
#  error "epoll_multiplexer is only usable on linux systems"
#else

#  include "net/fwd.hpp"
#  include "util/fwd.hpp"

#  include "net/detail/event_handler.hpp"
#  include "net/detail/multiplexer_base.hpp"

#  include <array>
#  include <cstdint>
#  include <span>
#  include <vector>

#  include <sys/epoll.h>

namespace net::detail {

/// @brief Event multiplexer using Linux epoll backend.
/// Implements the multiplexer interface for Linux systems using the epoll
/// mechanism for scalable I/O event handling. Manages sockets and coordinates
/// with event handlers for read/write operations. Only available on Linux.
class epoll_multiplexer : public multiplexer_base {
public:
  /// @brief Maximum number of events returned by a single poll.
  static constexpr std::size_t max_events = 32;

  /// @brief Factory function type for creating event handlers.
  using manager_factory
    = std::function<event_handler_ptr(net::socket, multiplexer_base*)>;

private:
  using event_type = epoll_event;
  using mpx_fd = int;

  // Pollset types
  /// @brief Container for events returned from epoll.
  using pollset = std::array<event_type, max_events>;
  /// @brief Pending epoll modifications.
  using update_list = std::vector<event_type>;
  /// @brief View over event arrays.
  using event_span = std::span<event_type>;

public:
  // -- constructors, destructors ----------------------------------------------

  /// @brief Default constructs an epoll multiplexer.
  epoll_multiplexer() = default;

  /// @brief Destructs the epoll multiplexer and closes the epoll file
  /// descriptor.
  virtual ~epoll_multiplexer();

  /// @brief Copy constructor.
  epoll_multiplexer(const epoll_multiplexer& other) = default;

  /// @brief Move constructor.
  epoll_multiplexer(epoll_multiplexer&& other) noexcept = default;

  /// @brief Copy assignment.
  epoll_multiplexer& operator=(const epoll_multiplexer& other) = default;

  /// @brief Move assignment.
  epoll_multiplexer& operator=(epoll_multiplexer&& other) noexcept = default;

  /// @brief Initializes the epoll multiplexer with the given configuration.
  /// Creates an epoll file descriptor and sets up event monitoring.
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

  /// @brief Performs a single epoll_wait and processes all ready events.
  /// @param blocking If true, wait indefinitely for events; if false, return
  /// immediately.
  /// @return An error on failure, none on success.
  util::error poll_once(bool blocking) override;

private:
  /// @brief Dispatches all ready events to their associated handlers.
  /// @param events The events returned by epoll_wait.
  void handle_events(event_span events);

  /// @brief Unregisters a socket manager from the epoll set.
  /// @param handle The socket identifier.
  void del(socket handle);

  /// @brief Unregisters a socket manager using an iterator.
  /// @param it Iterator to the manager in the manager map.
  /// @return Iterator to the element following the erased element.
  manager_map::iterator del(manager_map::iterator it) override;

  /// @brief Modifies epoll monitoring settings for an existing file descriptor.
  /// @param fd The file descriptor to modify.
  /// @param op The epoll operation (EPOLL_CTL_MOD for modification).
  /// @param events The operations to monitor (read/write).
  void mod(int fd, int op, operation events);

  // Multiplexing variables
  mpx_fd mpx_fd_{invalid_socket_id}; ///< The epoll file descriptor
  pollset pollset_;                  ///< Buffer for events returned by epoll
  update_list update_cache_;         ///< Pending epoll modifications
};

} // namespace net::detail

#endif
