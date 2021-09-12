/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

namespace net {

class multiplexer {
public:
  /// Initializes the multiplexer.
  virtual detail::error init(socket_manager_factory_ptr factory) = 0;

  /// Creates a thread that runs this multiplexer indefinately.
  virtual void start() = 0;

  /// Shuts the multiplexer down!
  virtual void shutdown() = 0;

  /// Joins with the multiplexer.
  virtual void join() = 0;

  // -- Error Handling ---------------------------------------------------------

  virtual void handle_error(detail::error err) = 0;

  // -- Interface functions ----------------------------------------------------

  /// The main multiplexing loop.
  virtual detail::error poll_once(bool blocking) = 0;

  /// Adds a new fd to the multiplexer for operation `initial`.
  /// @warning This function is *NOT* thread-safe.
  virtual void add(socket_manager_ptr mgr, operation initial) = 0;

  /// Enables an operation `op` for socket manager `mgr`.
  virtual void enable(socket_manager&, operation op) = 0;

  /// Disables an operation `op` for socket manager `mgr`.
  /// If `mgr` is not registered for any operation after disabling it, it is
  /// removed if `remove` is set.
  virtual void disable(socket_manager& mgr, operation op, bool remove) = 0;
};

} // namespace net
