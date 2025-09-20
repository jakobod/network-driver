/**
 *  @author    Jakob Otto
 *  @file      multiplexer.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "net/manager.hpp"

#include <chrono>

namespace net {

class multiplexer {
public:
  multiplexer() noexcept = default;

  multiplexer(const multiplexer&)            = delete;
  multiplexer& operator=(const multiplexer&) = delete;
  multiplexer(multiplexer&&)                 = delete;
  multiplexer& operator=(multiplexer&&)      = delete;

  // -- Interface functions ----------------------------------------------------

  /// Creates a thread that runs this multiplexer indefinately.
  virtual void start() = 0;

  /// Shuts the multiplexer down!
  virtual void shutdown() = 0;

  /// Joins with the multiplexer.
  virtual void join() = 0;

  virtual util::error add(sockets::tcp_stream_socket handle) = 0;

  virtual util::error add(sockets::udp_datagram_socket handle) = 0;

  virtual std::uint64_t
  set_timeout(manager_ptr mgr, std::chrono::steady_clock::time_point tp)
    = 0;

  // -- Implemented functions --------------------------------------------------

  bool running() const noexcept { return running_; }

  std::uint16_t port() const noexcept { return port_; }

protected:
  ~multiplexer() = default;

  bool running_{false};
  std::uint16_t port_{0};
};

} // namespace net
