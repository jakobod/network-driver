/**
 *  @author    Jakob Otto
 *  @file      manager.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "net/operation.hpp"

#include "net/sockets/socket.hpp"

#include "util/byte_buffer.hpp"
#include "util/ref_counted.hpp"

#include <chrono>

namespace net {

/// Manages the lifetime of a socket and its events.
class manager : public util::ref_counted {
public:
  // -- Object construction ----------------------------------------------------

  /// Constructs a manager object
  manager(sockets::socket handle, multiplexer* mpx);

  /// Destructs a socket manager object
  ~manager() override;

  /// Deleted copy constructor
  manager(const manager&) = delete;

  /// Deleted copy assignment operator
  manager& operator=(const manager&) = delete;

  /// Move constructor
  manager(manager&& other) noexcept;

  /// Move assignment operator
  manager& operator=(manager&& other) noexcept;

  // -- properties -------------------------------------------------------------

  /// Returns a ptr to the multiplexer
  template <class MultiplexerType = multiplexer>
  MultiplexerType* mpx() const noexcept {
    return reinterpret_cast<MultiplexerType*>(mpx_);
  }

  /// Returns the handle.
  template <class Socket = sockets::socket>
  constexpr Socket handle() const noexcept {
    return socket_cast<Socket>(handle_);
  }

  util::byte_buffer& write_buffer() noexcept;

  // -- Operation registration -------------------------------------------------

  /// Returns registered operations (read, write, or both).
  operation mask() const noexcept;

  /// Sets the event mask to the given flag(s).
  void mask_set(operation flag) noexcept;

  /// Adds given flag(s) to the event mask.
  bool mask_add(operation flag) noexcept;

  /// Tries to clear given flag(s) from the event mask.
  bool mask_del(operation flag) noexcept;

  // -- Timeout handling -------------------------------------------------------

  /// Sets a timeout in `duration` milliseconds with the id `timeout_id`
  uint64_t set_timeout_in(std::chrono::milliseconds duration);

  /// Sets a timeout at timepoint `point` with the id `timeout_id`
  uint64_t set_timeout_at(std::chrono::steady_clock::time_point timepoint);

  /// Handles a timeout-event
  virtual event_result handle_timeout(std::uint64_t timeout_id) = 0;

  // Operation registration ----------------------------------------------------

  void configure_next_read(receive_policy policy) noexcept;

  virtual void start_reading() = 0;

  virtual void start_writing() = 0;

private:
  /// The managed socket
  sockets::socket handle_{sockets::invalid_socket};
  /// Pointer to the multiplexer owning this socket-manager
  multiplexer* mpx_{nullptr};
  /// The mask containing all currently registered events
  operation mask_{operation::none};

protected:
  util::byte_buffer write_buffer_;
  std::size_t written_{0};

  util::byte_buffer read_buffer_;
  std::size_t received_{0};
  std::size_t min_read_size_{0};
};

using manager_ptr = util::intrusive_ptr<manager>;

} // namespace net
