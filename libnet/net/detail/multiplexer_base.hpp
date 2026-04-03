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

class multiplexer_base {
  friend class manager_base;

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

  multiplexer_base(const multiplexer_base& other) = delete;
  multiplexer_base(multiplexer_base&& other) noexcept = default;
  multiplexer_base& operator=(const multiplexer_base& other) = delete;
  multiplexer_base& operator=(multiplexer_base&& other) noexcept = default;

  /// Initializes the multiplexer_base.
  template <class ManagerBase>
  util::error init(acceptor<ManagerBase>::manager_factory factory,
                   const util::config& cfg) {
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
    add(util::make_intrusive<pollset_updater<ManagerBase>>(pipe_reader_, this),
        operation::read);

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

    static constexpr auto initial = std::invoke([]() constexpr {
#if defined(LIB_NET_URING)
      if constexpr (std::is_same_v<ManagerBase, uring_manager>) {
        return operation::accept;
      } else {
        return operation::read;
      }
#endif
      return operation::read;
    });
    add(util::make_intrusive<acceptor<ManagerBase>>(accept_socket, this,
                                                    std::move(factory)),
        initial);
    return util::none;
  }

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

  const util::config& cfg() const noexcept { return *cfg_; }

  bool shutting_down() const noexcept { return shutting_down_; }

protected:
  manager_base_ptr& add(manager_base_ptr mgr) {
    auto [it, success] = managers_.emplace(mgr->handle().id, std::move(mgr));
    return it->second;
  }

  virtual void del(net::socket handle) { managers_.erase(handle.id); }

  virtual manager_map::iterator del(manager_map::iterator it) {
    return managers_.erase(it);
  }

  template <class Manager = manager_base>
  const Manager* manager(net::socket handle) const noexcept {
    auto it = managers_.find(handle.id);
    if (it != managers_.end()) {
      return static_cast<Manager*>(it->second.get());
    }
    return nullptr;
  }

  template <class Manager = manager_base>
  Manager* manager(net::socket handle) noexcept {
    return const_cast<Manager*>(std::as_const(*this).manager<Manager>(handle));
  }

  bool has_managers() const noexcept { return !managers_.empty(); }

public:
  // -- Event handling interface -----------------------------------------------

  virtual void add(manager_base_ptr mgr, operation initial) = 0;

private:
  /// Enables an operation `op` for socket manager `mgr`.
  virtual void enable(manager_base& mgr, operation op) = 0;

  /// Disables an operation `op` for socket manager `mgr`.
  /// If `mgr` is not registered for any operation after disabling it, it is
  /// removed if `remove` is set.
  virtual void disable(manager_base& mgr, operation op, bool remove) = 0;

  /// Main multiplexing loop.
  virtual util::error poll_once(bool blocking) = 0;

  // -- Timeout management -----------------------------------------------------

  std::uint64_t set_timeout(manager_base& mgr,
                            std::chrono::steady_clock::time_point when);

protected:
  void handle_timeouts();

  // -- Error handling ---------------------------------------------------------

  virtual void handle_error(const util::error& err);

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

private:
  uint16_t port_{0};
  manager_map managers_;
  const util::config* cfg_{nullptr};

  // thread context
  std::thread mpx_thread_;
  std::thread::id mpx_thread_id_;

protected:
  bool shutting_down_{false};
  bool running_{false};

private:
  // pipe for synchronous access to mpx
  pipe_socket pipe_writer_{invalid_socket_id};
  pipe_socket pipe_reader_{invalid_socket_id};

  // timeout handling
  timeout_entry_set timeouts_;
  std::uint64_t current_timeout_id_{0};

protected:
  optional_timepoint current_timeout_{std::nullopt};
};

} // namespace net::detail
