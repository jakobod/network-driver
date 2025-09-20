/**
 *  @author    Jakob Otto
 *  @file      uring_multiplexer.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "net/multiplexer.hpp"
#include "net/timeout_entry.hpp"
#include "net/uring_manager.hpp"

#include "net/sockets/socket.hpp"
#include "net/sockets/socket_id.hpp"
#include "net/sockets/tcp_accept_socket.hpp"
#include "net/sockets/udp_datagram_socket.hpp"

#include "util/byte_buffer.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <liburing.h>
#include <thread>
#include <unordered_map>

namespace net {

/// Implements a multiplexing backend for handling event multiplexing facilities
/// such as epoll and kqueue.
class uring_multiplexer : public multiplexer {
  // Pollset types
  using manager_map = std::unordered_map<sockets::socket_id, uring_manager_ptr>;

  struct uring_entry {
    operation op{operation::none};
    uring_manager_ptr mgr_;
  };

public:
  // -- constructors, destructors ----------------------------------------------

  uring_multiplexer(manager_factory_ptr manager_factory);

  ~uring_multiplexer();

  /// Initializes the multiplexer.
  util::error init(const util::config& cfg);

  // -- Multiplexer interface --------------------------------------------------

  /// Creates a thread that runs this multiplexer indefinately.
  void start() override;

  /// Shuts the multiplexer down!
  void shutdown() override;

  /// Joins with the multiplexer.
  void join() override;

  util::error add(sockets::tcp_stream_socket handle) override;

  util::error add(sockets::udp_datagram_socket handle) override;

  std::uint64_t set_timeout(manager_ptr mgr,
                            std::chrono::steady_clock::time_point tp) override;

  // -- members ----------------------------------------------------------------

  std::uint16_t num_managers() const { return managers_.size(); }

  // -- Interface functions ----------------------------------------------------

  void enqueue_accept();

  void enqueue_read(uring_manager_ptr mgr, util::byte_span receive_buffer);

  void enqueue_write(uring_manager_ptr mgr, util::const_byte_span write_buffer);

private:
  /// The main multiplexer loop.
  void run();

  util::error poll_once(bool blocking);

  /// Deletes an existing socket_manager using its key `handle`.
  void del(sockets::socket handle);

  /// Deletes an existing socket_manager using an iterator `it` to the
  /// manager_map.
  manager_map::iterator del(manager_map::iterator it);

  // Multiplexing variables
  io_uring uring_handle_;
  manager_map managers_;
  manager_factory_ptr manager_factory_;

  // thread variables
  bool shutting_down_{false};
  std::jthread mpx_thread_;

  const util::config* cfg_ = nullptr;

  sockets::tcp_accept_socket accept_socket_;
};

// util::error_or<multiplexer_ptr>
// make_uring_multiplexer(socket_manager_factory_ptr factory,
//                        const util::config& cfg);

} // namespace net
