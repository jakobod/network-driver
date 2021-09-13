/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <array>
#include <sys/epoll.h>
#include <thread>
#include <unordered_map>

#include "fwd.hpp"
#include "net/multiplexer.hpp"
#include "net/pipe_socket.hpp"
#include "net/pollset_updater.hpp"

namespace net {

static const size_t max_epoll_events = 32;

class epoll_multiplexer : public multiplexer {
  using pollset = std::array<epoll_event, max_epoll_events>;
  using epoll_fd = int;
  using manager_map = std::unordered_map<int, socket_manager_ptr>;

public:
  // -- constructors, destructors ----------------------------------------------

  epoll_multiplexer();

  ~epoll_multiplexer();

  /// Initializes the multiplexer.
  error init(socket_manager_factory_ptr factory) override;

  // -- Thread functions -------------------------------------------------------

  /// Creates a thread that runs this multiplexer indefinately.
  void start() override;

  /// Shuts the multiplexer down!
  void shutdown() override;

  /// Joins with the multiplexer.
  void join() override;

  void set_thread_id();

  // -- Error Handling ---------------------------------------------------------

  void handle_error(error err) override;

  // -- Interface functions ----------------------------------------------------

  /// Main multiplexing loop.
  error poll_once(bool blocking) override;

  /// Adds a new fd to the multiplexer for operation `initial`.
  void add(socket_manager_ptr mgr, operation initial) override;

  void enable(socket_manager&, operation op) override;

  /// Disables an operation `op` for socket manager `mgr`.
  /// If `mgr` is not registered for any operation after disabling it, it is
  /// removed if `remove` is set.
  void disable(socket_manager& mgr, operation op, bool remove = true) override;

  // -- members ----------------------------------------------------------------

  uint16_t port() const noexcept {
    return listening_port_;
  }

  uint16_t num_socket_managers() const {
    return managers_.size();
  }

  bool running() const noexcept {
    return mpx_thread_.joinable();
  }

private:
  /// The main multiplexer loop.
  void run();

  /// Deletes an existing socket_manager using its key `handle`.
  void del(socket handle);

  /// Deletes an existing socket_manager using an iterator `it` to the
  /// manager_map.
  manager_map::iterator del(manager_map::iterator it);

  /// Modifies the epollset for existing fds.
  void mod(int fd, int op, operation events);

  // Network members
  uint16_t listening_port_;

  // pipe for synchronous access to mpx
  pipe_socket pipe_writer_;
  pipe_socket pipe_reader_;

  // epoll variables
  epoll_fd epoll_fd_;
  pollset pollset_;
  manager_map managers_;

  // thread variables
  bool shutting_down_;
  bool running_;
  std::thread mpx_thread_;
  std::thread::id mpx_thread_id_;
};

error_or<multiplexer_ptr>
make_epoll_multiplexer(socket_manager_factory_ptr factory) {
  auto mpx = std::make_shared<epoll_multiplexer>();
  if (auto err = mpx->init(std::move(factory)))
    return err;
  return mpx;
}

} // namespace net
