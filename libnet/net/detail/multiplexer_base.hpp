/**
 *  @author    Jakob Otto
 *  @file      multiplexer_base.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "net/detail/acceptor.hpp"
#include "net/detail/manager_base.hpp"
#include "net/detail/pollset_updater.hpp"
#include "net/detail/uring_manager.hpp"

#include "net/operation.hpp"
#include "net/socket/pipe_socket.hpp"
#include "net/socket/socket_id.hpp"
#include "net/timeout_entry.hpp"

#include "net/socket/tcp_accept_socket.hpp"

#include "net/ip/v4_address.hpp"
#include "net/ip/v4_endpoint.hpp"

#include "util/binary_serializer.hpp"
#include "util/byte_buffer.hpp"
#include "util/config.hpp"
#include "util/error.hpp"
#include "util/error_or.hpp"

#include <chrono>
#include <functional>
#include <set>
#include <thread>
#include <unordered_map>

namespace net::detail {

/// @brief Abstract base class for event-driven multiplexers.
/// Defines the interface and common functionality for socket management and
/// event handling. Concrete implementations (epoll, kqueue, uring) override
/// the event-specific methods. Manages socket managers, timeouts, and provides
/// thread-safe operation queueing via pipes.
class multiplexer_base {
  friend class manager_base;

protected:
  /// @brief Container type for socket managers.
  using manager_map = std::unordered_map<socket_id, manager_base_ptr>;

  // Timeout handling types
  /// @brief Optional time point for timeout tracking.
  using optional_timepoint
    = std::optional<std::chrono::steady_clock::time_point>;
  /// @brief Set of scheduled timeouts.
  using timeout_entry_set = std::set<timeout_entry>;

public:
  // -- constructors, destructors, initialization ----------------------------

  /// @brief Default constructs a multiplexer base.
  multiplexer_base() = default;

  /// @brief Virtual destructor.
  virtual ~multiplexer_base() = default;

  /// @brief Copy construction is deleted.
  multiplexer_base(const multiplexer_base& other) = delete;

  /// @brief Move constructor.
  multiplexer_base(multiplexer_base&& other) noexcept = default;

  /// @brief Copy assignment is deleted.
  multiplexer_base& operator=(const multiplexer_base& other) = delete;

  /// @brief Move assignment.
  multiplexer_base& operator=(multiplexer_base&& other) noexcept = default;

  /// @brief Initializes the multiplexer with factory and configuration.
  /// Creates the acceptance socket listening on the configured address and
  /// port, sets up the pollset updater pipe for thread-safe operations, and
  /// prepares the event handling infrastructure.
  /// @tparam ManagerBase The concrete manager type to use.
  /// @param factory Factory function for creating managers for accepted
  /// connections.
  /// @param cfg Configuration parameters (address, port, etc.).
  /// @return Error on failure, none on success.
  template <class ManagerBase>
  util::error init(acceptor_factory factory, const util::config& cfg) {
    LOG_TRACE();
    cfg_ = std::addressof(cfg);
    // Create pollset updater
    auto pipe_res = make_pipe();
    if (auto err = util::get_error(pipe_res)) {
      return *err;
    }
    auto pipe_fds = std::get<pipe_socket_pair>(pipe_res);
    pipe_reader_ = pipe_fds.first;
    pipe_writer_ = pipe_fds.second;
    {
      auto updater = util::make_intrusive<pollset_updater<ManagerBase>>(
        pipe_reader_, this);
      const auto initial = updater->initial_operation();
      add(std::move(updater), initial);
    }

    // Create Acceptor
    auto res = net::make_tcp_accept_socket(ip::v4_endpoint(
      (cfg.get_or("multiplexer.local", true) ? ip::v4_address::localhost
                                             : ip::v4_address::any),
      cfg.get_or<std::int64_t>("multiplexer.port", 0)));
    if (auto err = util::get_error(res)) {
      return *err;
    }
    auto accept_socket_pair = std::get<net::acceptor_pair>(res);
    auto accept_socket = accept_socket_pair.first;
    set_port(accept_socket_pair.second);

    {
      auto acc = util::make_intrusive<acceptor<ManagerBase>>(
        accept_socket, this, std::move(factory));
      const auto initial = acc->initial_operation();
      add(std::move(acc), initial);
    }
    return util::none;
  }

  // -- Thread functions ----------------------------------------------------

  /// @brief Creates and starts a thread running the multiplexer event loop
  /// indefinitely.
  void start();

private:
  /// @brief Main event loop function (runs in multiplexer thread).
  void run();

public:
  /// @brief Initiates graceful shutdown of the multiplexer.
  /// Signals the event loop to exit and stops accepting new connections.
  virtual void shutdown();

  /// @brief Blocks until the multiplexer thread completes.
  void join();

  /// @brief Returns whether the multiplexer is currently running.
  /// @return True if the event loop is active.
  bool is_running() const noexcept;

  /// @brief Sets the ID of the multiplexer thread (for debugging).
  /// @param tid The thread ID; defaults to empty/unset.
  void set_thread_id(std::thread::id tid = {}) noexcept;

  // -- members -------------------------------------------------------------

  /// @brief Returns the current number of active socket managers.
  /// @return The count of managed sockets.
  std::uint16_t num_socket_managers() const noexcept {
    return managers_.size();
  }

  /// @brief Returns the port the multiplexer is listening on.
  /// @return The listening port number.
  uint16_t port() const noexcept { return port_; }

  /// @brief Returns a reference to the configuration used to initialize this
  /// multiplexer.
  /// @return Const reference to the config.
  const util::config& cfg() const noexcept { return *cfg_; }

  /// @brief Returns whether the multiplexer is shutting down.
  /// @return True if shutdown has been initiated.
  bool shutting_down() const noexcept { return shutting_down_; }

protected:
  /// @brief Adds a manager to the internal registry without event registration.
  /// Must be followed by a call to add(manager_ptr, operation) for event
  /// handling.
  /// @param mgr The manager to register.
  /// @return Reference to the registered manager.
  manager_base_ptr& add(manager_base_ptr mgr) {
    auto [it, success] = managers_.emplace(mgr->handle().id, std::move(mgr));
    return it->second;
  }

  /// @brief Removes a manager from the registry by socket handle.
  /// @param handle The socket to remove.
  virtual void del(net::socket handle) { managers_.erase(handle.id); }

  /// @brief Removes a manager from the registry by iterator.
  /// @param it Iterator to the manager to remove.
  /// @return Iterator to the element following the erased element.
  virtual manager_map::iterator del(manager_map::iterator it) {
    return managers_.erase(it);
  }

  /// @brief Retrieves a manager by socket handle with type casting.
  /// @tparam Manager The typed manager class.
  /// @param handle The socket identifier.
  /// @return Pointer to the manager, or nullptr if not found.
  template <class Manager = manager_base>
  const Manager* manager(net::socket handle) const noexcept {
    auto it = managers_.find(handle.id);
    if (it != managers_.end()) {
      return static_cast<Manager*>(it->second.get());
    }
    return nullptr;
  }

  /// @brief Mutable version of manager retrieval.
  /// @tparam Manager The typed manager class.
  /// @param handle The socket identifier.
  /// @return Mutable pointer to the manager, or nullptr if not found.
  template <class Manager = manager_base>
  Manager* manager(net::socket handle) noexcept {
    return const_cast<Manager*>(std::as_const(*this).manager<Manager>(handle));
  }

  /// @brief Checks if any managers are currently registered.
  /// @return True if managers exist.
  bool has_managers() const noexcept { return !managers_.empty(); }

public:
  // -- Event handling interface -------------------------------------------

  /// @brief Registers a socket manager and enables initial operations.
  /// Concrete subclasses implement backend-specific registration logic.
  /// @param mgr The manager to register.
  /// @param initial The operations to monitor immediately (read/write/accept).
  virtual void add(manager_base_ptr mgr, operation initial) = 0;

private:
  /// @brief Enables monitoring of a specific operation for a manager.
  /// Subclasses implement backend-specific operation enabling.
  /// @param mgr The manager to modify.
  /// @param op The operation to enable (read/write).
  virtual void enable(manager_base& mgr, operation op) = 0;

  /// @brief Disables monitoring of a specific operation for a manager.
  /// Optionally removes the manager if no operations remain.
  /// @param mgr The manager to modify.
  /// @param op The operation to disable.
  /// @param remove If true, delete the manager when no operations remain.
  virtual void disable(manager_base& mgr, operation op, bool remove) = 0;

  /// @brief Performs a single event polling iteration.
  /// Subclasses implement backend-specific event retrieval and dispatch.
  /// @param blocking If true, wait for events; if false, return immediately.
  /// @return Error on failure, none on success.
  virtual util::error poll_once(bool blocking) = 0;

  // -- Timeout management -------------------------------------------------

  /// @brief Schedules a timeout callback for a manager at a specific time
  /// point.
  /// @param mgr The manager that requested the timeout.
  /// @param when The time point at which the timeout should fire.
  /// @return A timeout ID for later cancellation.
  std::uint64_t set_timeout(manager_base& mgr,
                            std::chrono::steady_clock::time_point when);

protected:
  /// @brief Processes all timeouts that have expired.
  void handle_timeouts();

  // -- Error handling ---------------------------------------------------

  /// @brief Handles errors from event processing.
  /// Called when event operations fail; subclasses may override for custom
  /// handling.
  /// @param err The error that occurred.
  virtual void handle_error(const util::error& err);

  /// @brief Checks whether the current thread is the multiplexer thread.
  /// @return True if running in the multiplexer thread.
  bool is_multiplexer_thread() {
    return (std::this_thread::get_id() == mpx_thread_id_);
  }

  /// @brief Writes binary data to the synchronization pipe for thread-safe
  /// operations. Allows external threads to queue operations safely.
  /// @param ts Arguments to serialize and send.
  /// @return Number of bytes written, or negative on error.
  template <class... Ts>
  ptrdiff_t write_to_pipe(Ts&&... ts) {
    util::byte_buffer buf;
    util::binary_serializer bs{buf};
    bs(std::forward<Ts>(ts)...);
    return write(pipe_writer_, util::as_const_bytes(buf));
  }

  /// @brief Internal helper to record the listening port.
  /// @param port The port number.
  void set_port(uint16_t port) noexcept { port_ = port; }

private:
  uint16_t port_{0};                 ///< Listening port
  manager_map managers_;             ///< Active socket managers
  const util::config* cfg_{nullptr}; ///< Configuration reference

  // thread context
  std::thread mpx_thread_;        ///< The multiplexer thread
  std::thread::id mpx_thread_id_; ///< ID of multiplexer thread

protected:
  bool shutting_down_{false}; ///< Shutdown flag
  bool running_{false};       ///< Running flag

private:
  // pipe for synchronous access to mpx
  pipe_socket pipe_writer_{invalid_socket_id}; ///< Write end of sync pipe
  pipe_socket pipe_reader_{invalid_socket_id}; ///< Read end of sync pipe

  // timeout handling
  timeout_entry_set timeouts_;          ///< Scheduled timeouts
  std::uint64_t current_timeout_id_{0}; ///< Next timeout ID

protected:
  optional_timepoint current_timeout_{std::nullopt}; ///< Nearest timeout
};

} // namespace net::detail
