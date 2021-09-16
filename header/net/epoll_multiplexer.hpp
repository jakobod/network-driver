/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <array>
#include <optional>
#include <queue>
#include <sys/epoll.h>
#include <thread>
#include <unordered_map>

#include "fwd.hpp"
#include "net/multiplexer.hpp"
#include "net/pipe_socket.hpp"
#include "net/pollset_updater.hpp"
#include "net/socket_manager.hpp"

namespace net {

static const size_t max_epoll_events = 32;

struct timeout_entry {
  net::socket_manager* mgr;
  std::chrono::system_clock::time_point when;
  uint64_t id;

  bool operator<(const timeout_entry& other) const {
    return when < other.when;
  }

  bool operator>(const timeout_entry& other) const {
    return when > other.when;
  }

  bool operator==(const timeout_entry& foo) const {
    return (mgr == foo.mgr) && (when == foo.when) && (id == foo.id);
  }
};

class epoll_multiplexer : public multiplexer {
  using optional_timepoint
    = std::optional<std::chrono::system_clock::time_point>;
  using pollset = std::array<epoll_event, max_epoll_events>;
  using epoll_fd = int;
  using manager_map = std::unordered_map<int, socket_manager_ptr>;

public:
  // -- constructors, destructors ----------------------------------------------

  epoll_multiplexer();

  ~epoll_multiplexer();

  /// Initializes the multiplexer.
  error init(socket_manager_factory_ptr factory, uint16_t port) override;

  // -- Thread functions -------------------------------------------------------

  /// Creates a thread that runs this multiplexer indefinately.
  void start() override;

  /// Shuts the multiplexer down!
  void shutdown() override;

  /// Joins with the multiplexer.
  void join() override;

  bool running() override;

  void set_thread_id();

  // -- Error Handling ---------------------------------------------------------

  void handle_error(error err) override;

  // -- Interface functions ----------------------------------------------------

  /// Main multiplexing loop.
  error poll_once(bool blocking) override;

  /// Adds a new fd to the multiplexer for operation `initial`.
  void add(socket_manager_ptr mgr, operation initial) override;

  /// Enables an operation `op` for socket manager `mgr`.
  void enable(socket_manager&, operation op) override;

  /// Disables an operation `op` for socket manager `mgr`.
  /// If `mgr` is not registered for any operation after disabling it, it is
  /// removed if `remove` is set.
  void disable(socket_manager& mgr, operation op, bool remove = true) override;

  void set_timeout(socket_manager* mgr, uint64_t timeout_id,
                   std::chrono::system_clock::time_point when) override;

  // -- members ----------------------------------------------------------------

  uint16_t num_socket_managers() const {
    return managers_.size();
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

  void handle_timeouts();

  // pipe for synchronous access to mpx
  pipe_socket pipe_writer_;
  pipe_socket pipe_reader_;

  // epoll variables
  epoll_fd epoll_fd_ = invalid_socket_id;
  pollset pollset_;
  manager_map managers_;

  // timeout handling
  std::priority_queue<timeout_entry, std::vector<timeout_entry>,
                      std::greater<timeout_entry>>
    timeouts_;
  optional_timepoint current_timeout_ = std::nullopt;

  // thread variables
  bool shutting_down_ = false;
  bool running_ = false;
  std::thread mpx_thread_;
  std::thread::id mpx_thread_id_;
};

error_or<multiplexer_ptr>
make_epoll_multiplexer(socket_manager_factory_ptr factory, uint16_t port = 0);

} // namespace net
