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

#include "net/manager_base.hpp"

#include "net/event_result.hpp"

#include <cstdint>

namespace net {

/// Manages the pollset of the multiplexer implementation. Handles adding,
/// enabling, disabling, and shutting down the multiplexer in a thread-safe
/// manner.
class pollset_updater : public manager_base {
public:
  // -- member types -----------------------------------------------------------

  /// Type for the opcodes used by this pollset_updater
  using opcode = std::uint8_t;

  // -- constants --------------------------------------------------------------

  /// Opcode for adding a socket_manager to the pollset.
  static constexpr const opcode add_code = 0x00;
  /// Opcode for triggering a shutdown of the multiplexer.
  static constexpr const opcode shutdown_code = 0x01;

  // -- constructors, destructors, and assignment operators --------------------

  /// Constructs a pollset updater
  pollset_updater(net::pipe_socket read_handle, multiplexer_base* parent);

  // -- interface functions ----------------------------------------------------

  /// Handles a read event, managing the pollset afterwards
  event_result handle_read_event() override;
};

} // namespace net
