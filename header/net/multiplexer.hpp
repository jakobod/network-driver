/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 30.03.2021
 */

#pragma once

#include <array>
#include <cstring>
#include <functional>
#include <iostream>
#include <sys/epoll.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#include "net/fwd.hpp"
#include "net/operation.hpp"

namespace net {

static const size_t max_epoll_events = 32;

class multiplexer {
  using pollset = std::array<epoll_event, max_epoll_events>;
  using epoll_fd = int;
  using manager_map = std::unordered_map<int, socket_manager_ptr>;

public:
  // -- constructors, destructors ----------------------------------------------
  multiplexer();
  ~multiplexer();

  /// Initializes the multiplexer.
  void init();

  /// Creates a thread that runs this multiplexer indefinately.
  void start();

  // -- Interface functions ----------------------------------------------------

  /// Adds a new socket manager to the multiplexer.
  /// @warning Takes ownership of the passed `mgr`
  void register_new_socket_manager(socket_manager_ptr mgr);

  /// Registers `mgr` for read events.
  void register_reading(socket sock);

  /// Registers `mgr` for write events.
  void register_writing(socket sock);

private:
  /// Adds an fd for operation `op`.
  void add(int fd, operation op);

  /// Deletes an fd for operation `op`.
  void del(int fd, operation op);

  void mod(int fd, int op, operation events);

  void run();

  // epoll variables
  epoll_fd epoll_fd_;
  pollset pollset_;
  manager_map managers_;

  // thread variables
  bool running_;
  std::thread mpx_thread_;
  std::thread::id mpx_thread_id_;
};

} // namespace net
