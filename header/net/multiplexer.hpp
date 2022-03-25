/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"
#include "net/error.hpp"
#include "net/operation.hpp"
#include "net/tcp_stream_socket.hpp"

#include <chrono>
#include <string>

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

  /// Handles an error `err`.
  virtual void handle_error(error err) = 0;

  // -- Interface functions ----------------------------------------------------

  /// The main multiplexing loop.
  virtual error poll_once(bool blocking) = 0;

  /// Adds a new fd to the multiplexer for operation `initial`.
  /// @warning This function is *NOT* thread-safe.
  virtual void add(socket_manager_ptr mgr, operation initial) = 0;

  /// Adds a new fd to the multiplexer.
  void add(socket_manager_ptr) {
    // nop
  }

  void add(request_ptr) {
    // nop
  }

  /// Enables an operation `op` for socket manager `mgr`.
  /// @warning This function is *NOT* thread-safe.
  virtual void enable(socket_manager&, operation op) = 0;

  /// Disables an operation `op` for socket manager `mgr`.
  /// If `mgr` is not registered for any operation after disabling it, it is
  /// removed if `remove` is set.
  /// @warning This function is *NOT* thread-safe.
  virtual void disable(socket_manager& mgr, operation op, bool remove) = 0;

  /// Sets a timeout for socket_manager `mgr`, with id `timeout_id`, at
  /// timepoint `when`.
  /// @warning This function is *NOT* thread-safe.
  virtual void set_timeout(socket_manager& mgr, uint64_t timeout_id,
                           std::chrono::system_clock::time_point when)
    = 0;

  template <class Manager, class... Ts>
  error tcp_connect(std::string host, uint16_t port, net::operation initial_op,
                    Ts&&... xs) {
    auto sock = net::make_connected_tcp_stream_socket(std::move(host), port);
    if (auto err = get_error(sock))
      return *err;
    auto mgr = std::make_shared<Manager>(std::get<tcp_stream_socket>(sock),
                                         this, std::forward<Ts>(xs)...);
    if (auto err = mgr->init())
      return err;
    add(std::move(mgr), initial_op);
    return none;
  }
  // -- members ----------------------------------------------------------------

  /// Returns the port the multiplexer is listening on.
  [[nodiscard]] uint16_t port() const noexcept {
    return port_;
  }

protected:
  uint16_t port_ = 0;
};

} // namespace net
