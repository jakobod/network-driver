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

#include "net/acceptor.hpp"
#include "net/manager_base.hpp"
#include "net/operation.hpp"
#include "net/socket/pipe_socket.hpp"
#include "net/socket/socket_id.hpp"
#include "net/timeout_entry.hpp"

#include "util/binary_serializer.hpp"
#include "util/byte_buffer.hpp"

#include <chrono>
#include <set>
#include <thread>
#include <unordered_map>

namespace net {

class multiplexer_base {
protected:
  using manager_map = std::unordered_map<socket_id, manager_base_ptr>;

  // Timeout handling types
  using optional_timepoint
    = std::optional<std::chrono::steady_clock::time_point>;
  using timeout_entry_set = std::set<timeout_entry>;

public:
  // -- constructors, destructors, initialization ------------------------------

  multiplexer_base() = default;

  virtual ~multiplexer_base() = default;

  /// Initializes the multiplexer_base.
  util::error init(manager_factory factory, const util::config& cfg);

  // -- Thread functions -------------------------------------------------------

  /// Creates a thread that runs this multiplexer_base indefinitely.
  void start();

private:
  void run();

public:
  /// Shuts the multiplexer_base down!
  virtual void shutdown();

  /// Joins with the multiplexer_base.
  void join();

  bool is_running() const noexcept;

  void set_thread_id(std::thread::id tid = {}) noexcept;

  // -- members ----------------------------------------------------------------

  std::uint16_t num_socket_managers() const noexcept {
    return managers_.size();
  }

  uint16_t port() const noexcept { return port_; }

  // -- Event handling interface -----------------------------------------------

  virtual void add(manager_base_ptr mgr, operation initial) = 0;

private:
  /// Deletes an existing socket_manager using an iterator `it` to the
  /// manager_map.
  virtual manager_map::iterator del(manager_map::iterator it) = 0;

public:
  /// Enables an operation `op` for socket manager `mgr`.
  virtual void enable(manager_base& mgr, operation op) = 0;

  /// Disables an operation `op` for socket manager `mgr`.
  /// If `mgr` is not registered for any operation after disabling it, it is
  /// removed if `remove` is set.
  virtual void disable(manager_base& mgr, operation op, bool remove) = 0;

  /// Main multiplexing loop.
  virtual util::error poll_once(bool blocking) = 0;

  // -- Timeout management -----------------------------------------------------

  std::uint64_t set_timeout(manager_base_ptr mgr,
                            std::chrono::steady_clock::time_point when);

  void handle_timeouts();

  // -- Error handling ---------------------------------------------------------

  virtual void handle_error(const util::error& err);

protected:
  bool is_multiplexer_thread() {
    return (std::this_thread::get_id() == mpx_thread_id_);
  }

  /// Writes the pollset_update code to the pipe
  template <class... Ts>
  ptrdiff_t write_to_pipe(Ts&&... ts) {
    util::byte_buffer buf;
    util::binary_serializer bs{buf};
    bs(std::forward<Ts>(ts)...);
    return write(pipe_writer_, util::as_const_bytes(buf));
  }

  void set_port(uint16_t port) noexcept { port_ = port; }

protected:
  uint16_t port_{0};
  manager_map managers_;
  const util::config* cfg_{nullptr};

  // thread context
  std::thread mpx_thread_;
  std::thread::id mpx_thread_id_;
  bool shutting_down_{false};
  bool running_{false};

  // pipe for synchronous access to mpx
  pipe_socket pipe_writer_{invalid_socket_id};
  pipe_socket pipe_reader_{invalid_socket_id};

  // timeout handling
  timeout_entry_set timeouts_;
  optional_timepoint current_timeout_{std::nullopt};
  std::uint64_t current_timeout_id_{0};
};

} // namespace net
