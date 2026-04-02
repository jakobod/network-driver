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

#include "net/detail/event_handler.hpp"

#include <cstdint>

namespace net::detail {

/// Manages the pollset of the multiplexer implementation. Handles adding,
/// enabling, disabling, and shutting down the multiplexer in a thread-safe
/// manner.
class pollset_updater : public event_handler {
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
  pollset_updater(net::pipe_socket handle, multiplexer_base* mpx);

  virtual ~pollset_updater() = default;

  pollset_updater(const pollset_updater& other) = default;
  pollset_updater(pollset_updater&& other) noexcept = default;
  pollset_updater& operator=(const pollset_updater& other) = default;
  pollset_updater& operator=(pollset_updater&& other) noexcept = default;

  // -- interface functions ----------------------------------------------------

  /// Handles a read event, managing the pollset afterwards
  event_result handle_read_event() override;
};

} // namespace net::detail
