/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/fwd.hpp"

#include "net/multiplexer.hpp"
#include "net/operation.hpp"
#include "net/pipe_socket.hpp"
#include "net/pollset_updater.hpp"
#include "net/socket_manager.hpp"
#include "net/timeout_entry.hpp"

#include <array>
#include <optional>
#include <set>
#include <thread>
#include <unordered_map>

#if defined(__linux__)
#  define EPOLL_MPX
#  include <sys/epoll.h>
#elif defined(__APPLE__)
#  define KQUEUE_MPX
#  include <sys/event.h>
#endif

namespace net {

#if defined(EPOLL_MPX)

class multiplexer_impl : public multiplexer {
  static constexpr const size_t max_epoll_events = 32;

  using optional_timepoint
    = std::optional<std::chrono::system_clock::time_point>;
  using pollset = std::array<epoll_event, max_epoll_events>;
  using epoll_fd = int;
  using manager_map = std::unordered_map<int, socket_manager_ptr>;
  using timeout_entry_set = std::set<timeout_entry>;

public:
  // -- constructors, destructors ----------------------------------------------

  multiplexer_impl();

  ~multiplexer_impl() override;

  /// Initializes the multiplexer.
  util::error init(socket_manager_factory_ptr factory, uint16_t port) override;

  // -- Thread functions -------------------------------------------------------

  /// Creates a thread that runs this multiplexer indefinately.
  void start() override;

  /// Shuts the multiplexer down!
  void shutdown() override;

  /// Joins with the multiplexer.
  void join() override;

  bool running() const override;

  void set_thread_id();

  // -- members ----------------------------------------------------------------

  [[nodiscard]] uint16_t num_socket_managers() const {
    return managers_.size();
  }

  // -- Error Handling ---------------------------------------------------------

  void handle_error(const util::error& err) override;

  // -- Interface functions ----------------------------------------------------

  /// Adds a new fd to the multiplexer for operation `initial`.
  void add(socket_manager_ptr mgr, operation initial) override;

  /// Enables an operation `op` for socket manager `mgr`.
  void enable(socket_manager&, operation op) override;

  /// Disables an operation `op` for socket manager `mgr`.
  /// If `mgr` is not registered for any operation after disabling it, it is
  /// removed if `remove` is set.
  void disable(socket_manager& mgr, operation op, bool remove) override;

  uint64_t set_timeout(socket_manager& mgr,
                       std::chrono::system_clock::time_point when) override;

  /// Main multiplexing loop.
  util::error poll_once(bool blocking) override;

private:
  /// Notifies all socket managers about timeouts that have expired.
  void handle_timeouts();

  /// Handles all IO-events that occurred.
  void handle_events(int num_events);

  /// The main multiplexer loop.
  void run();

  /// Deletes an existing socket_manager using its key `handle`.
  void del(socket handle);

  /// Deletes an existing socket_manager using an iterator `it` to the
  /// manager_map.
  manager_map::iterator del(manager_map::iterator it);

  /// Modifies the epollset for existing fds.
  void mod(int fd, int op, operation events);

  // pipe for synchronous access to mpx
  pipe_socket pipe_writer_;
  pipe_socket pipe_reader_;

  // epoll variables
  epoll_fd epoll_fd_ = invalid_socket_id;
  pollset pollset_;
  manager_map managers_;

  // timeout handling
  timeout_entry_set timeouts_;
  optional_timepoint current_timeout_ = std::nullopt;
  uint64_t current_timeout_id_ = 0;

  // thread variables
  bool shutting_down_ = false;
  bool running_ = false;
  std::thread mpx_thread_;
  std::thread::id mpx_thread_id_;
};

#elif defined(KQUEUE_MPX)

class multiplexer_impl : public multiplexer {
  using kqueue_event = struct kevent;
  using kqueue_fd = int;
  using manager_map = std::unordered_map<socket_id, socket_manager_ptr>;

public:
  // -- constructors, destructors ----------------------------------------------

  multiplexer_impl();

  ~multiplexer_impl() override;

  /// Initializes the multiplexer.
  util::error init(socket_manager_factory_ptr factory, uint16_t port) override;

  // -- Thread functions -------------------------------------------------------

  /// Creates a thread that runs this multiplexer indefinately.
  void start() override;

  /// Shuts the multiplexer down!
  void shutdown() override;

  /// Joins with the multiplexer.
  void join() override;

  bool running() const override;

  void set_thread_id();

  // -- members ----------------------------------------------------------------

  [[nodiscard]] uint16_t num_socket_managers() const {
    return managers_.size();
  }

  // -- Error Handling ---------------------------------------------------------

  void handle_error(const util::error& err) override;

  // -- Interface functions ----------------------------------------------------

  /// Adds a new fd to the multiplexer for operation `initial`.
  void add(socket_manager_ptr mgr, operation initial) override;

  /// Enables an operation `op` for socket manager `mgr`.
  void enable(socket_manager&, operation op) override;

  /// Disables an operation `op` for socket manager `mgr`.
  /// If `mgr` is not registered for any operation after disabling it, it is
  /// removed if `remove` is set.
  void disable(socket_manager& mgr, operation op, bool remove) override;

  uint64_t set_timeout(socket_manager& mgr,
                       std::chrono::system_clock::time_point when) override;

  /// Main multiplexing loop.
  util::error poll_once(bool blocking) override;

private:
  /// Notifies all socket managers about timeouts that have expired.
  void handle_timeouts();

  /// Handles all IO-events that occurred.
  void handle_events(std::span<struct kevent> events);

  /// The main multiplexer loop.
  void run();

  /// Deletes an existing socket_manager using its key `handle`.
  void del(socket handle);

  /// Deletes an existing socket_manager using an iterator `it` to the
  /// manager_map.
  manager_map::iterator del(manager_map::iterator it);

  /// Modifies the epollset for existing fds.
  void mod(int fd, int op, operation events);

  // pipe for synchronous access to mpx
  pipe_socket pipe_writer_{invalid_socket_id};
  pipe_socket pipe_reader_{invalid_socket_id};

  // epoll variables
  kqueue_fd kqueue_fd_{invalid_socket_id};
  manager_map managers_;

  // thread variables
  bool shutting_down_{false};
  bool running_{false};
  std::thread mpx_thread_;
  std::thread::id mpx_thread_id_;
};

#else
#  error "No multiplexer backend implemented for the system!"
#endif

util::error_or<multiplexer_ptr>
make_multiplexer(socket_manager_factory_ptr factory, uint16_t port = 0);

} // namespace net
