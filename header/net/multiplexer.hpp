/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include <chrono>

namespace net {

class multiplexer {
public:
  /// Initializes the multiplexer.
  virtual error init(socket_manager_factory_ptr factory, uint16_t port) = 0;

  /// Creates a thread that runs this multiplexer indefinately.
  virtual void start() = 0;

  /// Shuts the multiplexer down!
  virtual void shutdown() = 0;

  /// Joins with the multiplexer.
  virtual void join() = 0;

  virtual bool running() = 0;

  // -- Error Handling ---------------------------------------------------------

  virtual void handle_error(error err) = 0;

  // -- Interface functions ----------------------------------------------------

  /// The main multiplexing loop.
  virtual error poll_once(bool blocking) = 0;

  /// Adds a new fd to the multiplexer for operation `initial`.
  /// @warning This function is *NOT* thread-safe.
  virtual void add(socket_manager_ptr mgr, operation initial) = 0;

  /// Enables an operation `op` for socket manager `mgr`.
  virtual void enable(socket_manager&, operation op) = 0;

  /// Disables an operation `op` for socket manager `mgr`.
  /// If `mgr` is not registered for any operation after disabling it, it is
  /// removed if `remove` is set.
  virtual void disable(socket_manager& mgr, operation op, bool remove) = 0;

  virtual void set_timeout(socket_manager* mgr, uint64_t timeout_id,
                           std::chrono::system_clock::time_point)
    = 0;

  // -- members ----------------------------------------------------------------

  uint16_t port() const noexcept {
    return port_;
  }

protected:
  uint16_t port_ = 0;
};

} // namespace net
