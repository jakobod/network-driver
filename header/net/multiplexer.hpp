/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/fwd.hpp"

#include "net/ip/v4_endpoint.hpp"

#include "net/operation.hpp"
#include "net/tcp_stream_socket.hpp"

#include "util/error.hpp"

#include <chrono>
#include <string>

namespace net {

class multiplexer {
public:
  virtual ~multiplexer() = default;

  /// Initializes the multiplexer.
  virtual util::error init(socket_manager_factory_ptr factory, uint16_t port)
    = 0;

  /// Creates a thread that runs this multiplexer indefinately.
  virtual void start() = 0;

  /// Shuts the multiplexer down!
  virtual void shutdown() = 0;

  /// Joins with the multiplexer.
  virtual void join() = 0;

  virtual bool running() const = 0;

  // -- Error Handling ---------------------------------------------------------

  /// Handles an error `err`.
  virtual void handle_error(const util::error& err) = 0;

  // -- Interface functions ----------------------------------------------------

  /// The main multiplexing loop.
  virtual util::error poll_once(bool blocking) = 0;

  /// Adds a new fd to the multiplexer for operation `initial`.
  /// @warning This function is *NOT* thread-safe.
  virtual void add(socket_manager_ptr mgr, operation initial) = 0;

  /// Enables an operation `op` for socket manager `mgr`.
  /// @warning This function is *NOT* thread-safe.
  virtual void enable(socket_manager&, operation op) = 0;

  /// Disables an operation `op` for socket manager `mgr`.
  /// If `mgr` is not registered for any operation after disabling it, it is
  /// removed if `remove` is set.
  /// @warning This function is *NOT* thread-safe.
  virtual void disable(socket_manager& mgr, operation op, bool remove) = 0;

  /// Sets a timeout for socket_manager `mgr` at timepoint `when` and returns
  /// the id.
  /// @warning This function is *NOT* thread-safe.
  virtual uint64_t
  set_timeout(socket_manager& mgr, std::chrono::system_clock::time_point when)
    = 0;

  template <class Manager, class... Ts>
  util::error
  tcp_connect(const ip::v4_endpoint& ep, operation initial_op, Ts&&... xs) {
    auto sock = make_connected_tcp_stream_socket(ep);
    if (auto err = get_error(sock))
      return *err;
    auto mgr = std::make_shared<Manager>(std::get<tcp_stream_socket>(sock),
                                         this, std::forward<Ts>(xs)...);
    if (auto err = mgr->init())
      return err;
    add(std::move(mgr), initial_op);
    return util::none;
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
