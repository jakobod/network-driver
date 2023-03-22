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

#include "net/socket_manager.hpp"

#include <cstdint>

namespace net {

/// Manages the pollset of the multiplexer implementation. Handles adding,
/// enabling, disabling, and shutting down the multiplexer in a thread-safe
/// manner.
class pollset_updater : public socket_manager {
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
  pollset_updater(pipe_socket read_handle, multiplexer* parent);

  /// Initializes the pollset updater.
  util::error init(const util::config& cfg) override;

  // -- interface functions ----------------------------------------------------

  /// Handles a read event, managing the pollset afterwards
  event_result handle_read_event() override;

  /// Handles a write event.
  event_result handle_write_event() override;

  /// Handles a timeout event
  event_result handle_timeout(uint64_t timeout_id) override;
};

} // namespace net
