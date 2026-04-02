/**
 *  @author    Jakob Otto
 *  @file      multiplexer_impl.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released
 under
 *             the GNU GPL3 License.
 */

#pragma once

#if !defined(__linux__)
#  error "uring multiplexer is only usable on Linux"
#elif !defined(LIB_NET_URING)
#  error "uring support not enabled"
#else

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

/// Implements a multiplexing backend for handling event multiplexing facilities
/// such as epoll and kqueue.
class uring_multiplexer : public multiplexer_base {
  static constexpr std::size_t max_uring_depth = 32;

public:
  using manager_factory
    = std::function<uring_manager_ptr(net::socket, uring_multiplexer*)>;

  // -- constructors, destructors ----------------------------------------------

  uring_multiplexer() = default;

  virtual ~uring_multiplexer();

  /// Initializes the multiplexer.
  util::error init(manager_factory factory, const util::config& cfg);

  // -- Interface function -----------------------------------------------------

  void add(manager_base_ptr mgr, operation initial) override;

private:
  /// Deletes an existing socket_manager using an iterator `it` to the
  /// manager_map.
  manager_map::iterator del(manager_map::iterator it) override;

  /// Enables an operation `op` for socket manager `mgr`.
  void enable(manager_base& mgr, operation op) override;

  /// Disables an operation `op` for socket manager `mgr`.
  /// If `mgr` is not registered for any operation after disabling it, it is
  /// removed if `remove` is set.
  void disable(manager_base& mgr, operation op, bool remove) override;

  /// Main multiplexing loop.
  util::error poll_once(bool blocking) override;

  void handle_events();

  // Multiplexing variables
  io_uring uring_;
};

using uring_multiplexer_ptr = std::shared_ptr<uring_multiplexer>;

util::error_or<uring_multiplexer_ptr>
make_uring_multiplexer(uring_multiplexer::manager_factory factory,
                       const util::config& cfg);

} // namespace net::detail

#endif
