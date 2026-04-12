/**
 *  @author    Jakob Otto
 *  @file      pollset_updater.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

// TODO: Handle multiple operations at once?

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#if defined(LIB_NET_URING)
#  include "net/detail/uring_manager.hpp"
#endif

#include "net/detail/event_handler.hpp"

#include "net/manager_result.hpp"
#include "net/operation.hpp"

#include "net/socket/pipe_socket.hpp"

#include "util/binary_deserializer.hpp"
#include "util/byte_buffer.hpp"
#include "util/byte_span.hpp"
#include "util/error.hpp"
#include "util/logger.hpp"

#include <cstdint>
#include <sys/socket.h>
#include <sys/uio.h>

namespace net::detail {

/// @brief Opcodes for commands sent through the pipe to the pollset updater.
/// Used to signal different operations that need to be performed on the
/// multiplexer's pollset.
enum class pollset_opcode : std::uint8_t {
  /// No operation (unused)
  none = 0x00,
  /// Opcode indicating a new socket manager should be added to the pollset.
  add = 0x01,
  /// Opcode indicating the multiplexer should be shut down.
  shutdown = 0x02,
};

/// @brief Generic base for pollset updater implementations.
/// Manages the pollset of the multiplexer from a separate thread. Handles
/// adding and removing socket managers, and coordinating multiplexer
/// shutdown in a thread-safe manner via a pipe socket.
///
/// The pollset_updater runs in the multiplexer thread and reads commands
/// from a pipe that may be written to from other threads, allowing
/// safe modification of the pollset without race conditions.
template <class ManagerBase>
class pollset_updater_base : public ManagerBase {
public:
  static constexpr std::size_t message_size
    = sizeof(pollset_opcode) + sizeof(manager_base*) + sizeof(operation);

  // -- constructors, destructors, and assignment operators --------------------

  /// @brief Constructs a pollset updater.
  /// @param handle The read end of the pipe socket for receiving commands.
  /// @param mpx Pointer to the owning multiplexer.
  pollset_updater_base(net::pipe_socket handle, multiplexer_base* mpx);

  /// @brief Destructs the pollset updater.
  virtual ~pollset_updater_base() = default;

protected:
  // -- protected helper functions for common operations ----------------------

  /// @brief Handles a pollset update operation received from the pipe.
  /// Processes the opcode and updates the pollset accordingly.
  /// @return The result of the operation.
  manager_result handle_operation();

  pollset_opcode code_;
  manager_base* mgr_;
  operation op_;
  std::array<iovec, 3> iov_;
  msghdr msg_;
};

template <class ManagerBase>
class pollset_updater;

/// @brief Specialization for epoll/kqueue-based managers.
/// Handles pollset updates via read events on the pipe socket.
/// When the pipe has data to read, it decodes the command and processes
/// the pollset update accordingly.
template <>
class pollset_updater<event_handler>
  : public pollset_updater_base<event_handler> {
  using base = pollset_updater_base<event_handler>;

public:
  using base::base;

  /// @brief Handles a read event from the pipe socket (epoll/kqueue).
  /// Reads commands from the pipe and processes pollset updates.
  /// @return The result of handling the read event.
  manager_result handle_read_event();
};

#if defined(LIB_NET_URING)

/// @brief Specialization for io_uring-based managers.
/// Handles pollset updates via io_uring completion events on the pipe read.
/// When a completion event fires on the pipe socket, it decodes the command
/// and processes the pollset update accordingly.
template <>
class pollset_updater<uring_manager>
  : public pollset_updater_base<uring_manager> {
  using base = pollset_updater_base<uring_manager>;

public:
  pollset_updater(net::pipe_socket handle, multiplexer_base* mpx);

  manager_result enable(operation op) override;

  /// @brief Handles a completion event from the pipe socket (io_uring).
  /// Reads commands from the pipe and processes pollset updates.
  /// @param op The operation that completed (should be read).
  /// @param res The result of the io_uring operation (bytes read).
  /// @return The result of handling the completion event.
  manager_result handle_completion(operation op, int res) override;
};

#endif // LIB_NET_URING

} // namespace net::detail
