/**
 *  @author    Jakob Otto
 *  @file      multiplexer_impl.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released
 under
 *             the GNU GPL3 License.
 */

#pragma once

#if defined(LIB_NET_URING)

#  include "net/fwd.hpp"
#  include "util/fwd.hpp"

#  include "net/detail/multiplexer_base.hpp"
#  include "net/detail/uring_manager.hpp"

#  include <array>
#  include <cstdint>
#  include <functional>
#  include <span>
#  include <vector>

#  include <liburing.h>

namespace net::detail {

/// @brief Event multiplexer using Linux io_uring backend.
/// Implements the multiplexer interface for Linux systems using io_uring
/// for scalable I/O submission and completion handling. Only available when
/// LIB_NET_URING is defined during compilation.
class uring_multiplexer : public multiplexer_base {
  /// @brief Maximum queue depth for pending operations.
  static constexpr std::size_t max_uring_depth = 32;

public:
  /// @brief Factory function type for creating io_uring-specific managers.
  using manager_factory
    = std::function<uring_manager_ptr(net::socket, uring_multiplexer*)>;

  // -- constructors, destructors -------------------------------------------

  /// @brief Default constructs a uring multiplexer.
  uring_multiplexer() = default;

  /// @brief Destructs the uring multiplexer and closes the io_uring instance.
  virtual ~uring_multiplexer();

  /// @brief Initializes the uring multiplexer with the given configuration.
  /// Creates an io_uring and sets up event monitoring.
  /// @param factory Factory function for creating uring managers.
  /// @param cfg Configuration parameters for the multiplexer.
  /// @return An error on failure, none on success.
  util::error init(manager_factory factory, const util::config& cfg);

  // -- Interface functions --------------------------------------------------

  std::pair<bool, uint64_t> submit(uring_manager_ptr mgr, operation op);

  /// @brief Registers a socket manager for io_uring event monitoring.
  /// @param mgr The manager to register.
  /// @param initial The initial operations to monitor.
  void add(manager_base_ptr mgr, operation initial) override;

private:
  /// @brief Removes a manager from the registry using an iterator.
  /// @param it Iterator to the manager.
  /// @return Iterator to the element following the erased element.
  manager_map::iterator del(manager_map::iterator it) override;

  /// @brief Enables monitoring of an operation for a manager.
  /// @param mgr The manager to modify.
  /// @param op The operation to enable (read/write).
  void enable(manager_base& mgr, operation op) override;

  /// @brief Disables monitoring of an operation for a manager.
  /// @param mgr The manager to modify.
  /// @param op The operation to disable.
  /// @param remove If true, delete the manager when no operations remain.
  void disable(manager_base& mgr, operation op, bool remove) override;

public:
  /// @brief Performs a single io_uring submission and processes completions.
  /// @param blocking If true, wait for completions; if false, return
  /// immediately.
  /// @return An error on failure, none on success.
  util::error poll_once(bool blocking) override;

private:
  /// @brief Dispatches all completion queue entries to their handlers.
  void handle_events();

  // Multiplexing variables
  struct io_uring uring_ {}; ///< The io_uring instance

  std::uint64_t current_submission_id_{0};
};

/// @brief Shared pointer type for uring multiplexers.
using uring_multiplexer_ptr = std::shared_ptr<uring_multiplexer>;

/// @brief Factory function to create a uring multiplexer.
/// @param factory Factory function for creating managers.
/// @param cfg Configuration parameters.
/// @return The created multiplexer on success, or an error on failure.
util::error_or<uring_multiplexer_ptr>
make_uring_multiplexer(uring_multiplexer::manager_factory factory,
                       const util::config& cfg);

} // namespace net::detail

#endif
