/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 30.03.2021
 */

#pragma once

#include <array>
#include <sys/epoll.h>
#include <thread>
#include <unordered_map>

#include "detail/error.hpp"
#include "fwd.hpp"
#include "net/operation.hpp"
#include "net/pipe_socket.hpp"
#include "net/pollset_updater.hpp"

namespace net {

static const size_t max_epoll_events = 32;

class multiplexer {
  using pollset = std::array<epoll_event, max_epoll_events>;
  using epoll_fd = int;
  using manager_map = std::unordered_map<int, socket_manager_ptr>;

public:
  // -- constructors, destructors ----------------------------------------------

  multiplexer(benchmark::result_ptr results);

  ~multiplexer();

  /// Initializes the multiplexer.
  detail::error init(socket_manager_factory_ptr factory);

  /// Creates a thread that runs this multiplexer indefinately.
  void start();

  /// Shuts the multiplexer down!
  void shutdown();

  /// Joins with the multiplexer.
  void join();

  // -- Error Handling ---------------------------------------------------------

  void handle_error(detail::error err);

  // -- Interface functions ----------------------------------------------------

  /// Adds a new fd to the multiplexer for operation `initial`.
  void add(socket_manager_ptr mgr, operation initial);

  void enable(socket_manager&, operation op);

  /// Disables an operation `op` for socket manager `mgr`.
  /// If `mgr` is not registered for any operation after disabling it, it is
  /// removed if `remove` is set.
  void disable(socket_manager& mgr, operation op, bool remove = true);

private:
  /// Deletes an existing socket_manager using its key `handle`.
  void del(socket handle);

  /// Deletes an existing socket_manager using an iterator `it` to the
  /// manager_map.
  manager_map::iterator del(manager_map::iterator it);

  /// Modifies the epollset for existing fds.
  void mod(int fd, int op, operation events);

  /// Main multiplexing loop.
  void run();

  // Benchmark results
  benchmark::result_ptr results_;

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

} // namespace net
