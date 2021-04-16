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

  using msg_buf = std::array<std::byte, 1>;

  // -- constants --------------------------------------------------------------

  static constexpr opcode shutdown_code = 0x01;

  // -- constructors, destructors, and assignment operators --------------------

  pollset_updater(pipe_socket read_handle, multiplexer* parent);

  // -- interface functions ----------------------------------------------------

  bool handle_read_event() override;

  bool handle_write_event() override;

  virtual std::string me() const {
    return "pollset_updater";
  }
};

} // namespace net