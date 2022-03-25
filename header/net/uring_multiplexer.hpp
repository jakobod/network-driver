/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <array>
#include <liburing.h>
#include <optional>
#include <set>
#include <sys/epoll.h>
#include <thread>
#include <unordered_map>

#include "fwd.hpp"
#include "net/multiplexer.hpp"
#include "net/pipe_socket.hpp"
#include "net/pollset_updater.hpp"
#include "net/socket.hpp"
#include "net/socket_manager.hpp"
#include "net/timeout_entry.hpp"

namespace {

enum class event : uint8_t {
  accept = 0,
  read = 1,
  write = 2,
};

std::string to_string(event e) {
  switch (e) {
    case event::accept:
      return "event::accept";
    case event::read:
      return "event::read";
    case event::write:
      return "event::write";
    default:
      return std::to_string(static_cast<uint8_t>(e));
  }
}

struct request {
  request(event e, net::socket sock, size_t size)
    : ev{e}, sock{sock}, iov{new std::byte[size], size} {
    // nop
  }

  ~request() {
    delete[] reinterpret_cast<std::byte*>(iov.iov_base);
  }

  const event ev;
  const net::socket sock;
  const iovec iov;
};

using request_ptr = std::unique_ptr<request>;

} // namespace

namespace net {

class uring_multiplexer : multiplexer {
  static constexpr size_t max_uring_depth = 32;

  using manager_map = std::unordered_map<int, socket_manager_ptr>;

public:
  // -- constructors, destructors ----------------------------------------------

  uring_multiplexer();

  ~uring_multiplexer();

  /// Initializes the multiplexer.
  error init(socket_manager_factory_ptr factory, uint16_t port);

  // -- Thread functions -------------------------------------------------------

  /// Creates a thread that runs this multiplexer indefinately.
  void start();

  /// Shuts the multiplexer down!
  void shutdown();

  /// Joins with the multiplexer.
  void join();

  bool running();

  void set_thread_id();

  // -- Error Handling ---------------------------------------------------------

  void handle_error(error err);

  // -- Interface functions ----------------------------------------------------

  /// Main multiplexing loop.
  error poll_once(bool blocking);

  /// Adds a new fd to the multiplexer.
  void add(socket_manager_ptr mgr) override;

  void add(request_ptr req) override;

  // -- members ----------------------------------------------------------------

  uint16_t num_socket_managers() const {
    return managers_.size();
  }

private:
  error handle_accept(tcp_stream_socket handle);

  /// The main multiplexer loop.
  void run();

  /// Deletes an existing socket_manager using its key `handle`.
  void del(socket handle);

  /// Deletes an existing socket_manager using an iterator `it` to the
  /// manager_map.
  manager_map::iterator del(manager_map::iterator it);

  // pipe for synchronous access to mpx
  pipe_socket pipe_writer_;
  pipe_socket pipe_reader_;

  // Accept socket
  tcp_accept_socket accept_socket_{invalid_socket_id};
  socket_manager_factory_ptr factory_;

  // uring state
  io_uring uring;
  manager_map managers_;

  // thread variables
  bool shutting_down_ = false;
  bool running_ = false;
  std::thread mpx_thread_;
  std::thread::id mpx_thread_id_;
};

error_or<std::unique_ptr<uring_multiplexer>>
make_uring_multiplexer(socket_manager_factory_ptr factory, uint16_t port = 0);

} // namespace net
