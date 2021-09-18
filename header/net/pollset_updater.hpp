/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 07.04.2021
 */

#pragma once

#include <array>
#include <cstdint>

#include "fwd.hpp"
#include "net/socket_manager.hpp"

namespace net {

class pollset_updater : public socket_manager {
public:
  // -- member types -----------------------------------------------------------

  using opcode = uint8_t;

  // -- constants --------------------------------------------------------------

  static constexpr opcode add_code = 0x00;

  static constexpr opcode enable_code = 0x01;

  static constexpr opcode disable_code = 0x02;

  static constexpr opcode shutdown_code = 0x03;

  // -- constructors, destructors, and assignment operators --------------------

  pollset_updater(pipe_socket read_handle, multiplexer* parent);

  error init() override;

  // -- interface functions ----------------------------------------------------

  event_result handle_read_event() override;

  event_result handle_write_event() override;

  event_result handle_timeout(uint64_t timeout_id) override;
};

} // namespace net
