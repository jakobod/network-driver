/**
 *  @author    Jakob Otto
 *  @file      pollset_updater.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#if defined(LIB_NET_URING)
#  include "net/detail/uring_manager.hpp"
#endif

#include "net/event_result.hpp"

#include "net/socket/pipe_socket.hpp"

#include "util/binary_deserializer.hpp"
#include "util/byte_span.hpp"
#include "util/error.hpp"
#include "util/logger.hpp"

#include <cstdint>

namespace net::detail {

/// Manages the pollset of the multiplexer implementation. Handles adding,
/// enabling, disabling, and shutting down the multiplexer in a thread-safe
/// manner.
template <class Base>
class pollset_updater final : public Base {
public:
  // -- member types -----------------------------------------------------------

  /// Type for the opcodes used by this pollset_updater
  enum class opcode : std::uint8_t {
    none = 0x00,
    /// Opcode for adding a socket_manager to the pollset.
    add = 0x01,
    /// Opcode for triggering a shutdown of the multiplexer.
    shutdown = 0x02,
  };

  // -- constructors, destructors, and assignment operators --------------------

  /// Constructs a pollset updater
  pollset_updater(net::pipe_socket handle, multiplexer_base* mpx)
    : Base{handle, mpx} {
    LOG_TRACE();
#if defined(LIB_NET_URING)
    if constexpr (std::is_same_v<Base, detail::uring_manager>) {
      uring_manager::read_buffer().resize(10);
    }
#endif
  }

  virtual ~pollset_updater() = default;

  // -- interface functions ----------------------------------------------------

  /// Handles a read event, managing the pollset afterwards
  virtual event_result handle_read_event() {
    LOG_TRACE();
    opcode code = opcode::none;
    Base* mgr = nullptr;
    operation op;

    if (read_from_pipe(code)) {
      return event_result::error;
    }
    if (read_from_pipe(mgr)) {
      return event_result::error;
    }
    if (read_from_pipe(op)) {
      return event_result::error;
    }
    return handle_operation(code, mgr, op);
  }

  /// Handles a read event, managing the pollset afterwards
  virtual event_result handle_completion([[maybe_unused]] operation op,
                                         [[maybe_unused]] int res) {
#if defined(LIB_NET_URING)
    if constexpr (std::is_same_v<Base, detail::uring_manager>) {
      if (op != operation::read) {
        LOG_ERROR("pollset_updater called for operation != read");
        return event_result::error;
      }

      if (res != 10) {
        LOG_ERROR("Not enough bytes for pollset updater to use");
        return event_result::ok;
      }

      opcode code = opcode::none;
      Base* mgr = nullptr;
      operation op;
      util::binary_deserializer deserializer(Base::read_buffer());
      deserializer(code, mgr, op);
      return handle_operation(code, mgr, op);
    }
#endif
    return event_result::error;
  }

private:
  template <class T>
  util::error read_from_pipe(T& t) {
    if (const auto res = net::read(Base::template handle<pipe_socket>(),
                                   util::as_bytes(&t, sizeof(T)));
        res != sizeof(T)) {
      LOG_ERROR("Could not read ", sizeof(T), " bytes from pipe socket: ",
                net::last_socket_error_as_string());
      return util::error_code::runtime_error;
    }
    return util::none;
  }

  event_result handle_operation(opcode code, Base* mgr, operation op) {
    switch (code) {
      case opcode::add:
        LOG_DEBUG("Received opcode::add for mgr with ",
                  NET_ARG2("id", mgr->handle().id), " with ", NET_ARG(op));
        Base::mpx()->add(util::make_intrusive(mgr, false), op);
        return event_result::ok;

      case opcode::shutdown:
        LOG_DEBUG("Received opcode::shutdown");
        Base::mpx()->shutdown();
        return event_result::done;

      default:
        LOG_WARNING("Received unhandled code");
        return event_result::error;
    }
  }
}; // namespace net::detail

} // namespace net::detail
