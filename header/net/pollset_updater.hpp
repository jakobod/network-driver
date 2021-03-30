/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 30.03.2021
 *
 *  This file is based on `pollset_updater.hpp` from the C++ Actor Framework.
 *  https://github.com/actor-framework/incubator
 */

#pragma once

#include <array>
#include <cstdint>
#include <mutex>

#include "net/pipe_socket.hpp"
#include "net/socket_manager.hpp"

namespace net {

class pollset_updater : public socket_manager {
public:
  // -- member types -----------------------------------------------------------

  using super = socket_manager;

  using msg_buf = std::array<std::byte, sizeof(intptr_t) + 1>;

  // -- constants --------------------------------------------------------------

  static constexpr uint8_t register_reading_code = 0x00;

  static constexpr uint8_t register_writing_code = 0x01;

  static constexpr uint8_t init_manager_code = 0x02;

  static constexpr uint8_t shutdown_code = 0x04;

  // -- constructors, destructors, and assignment operators --------------------

  pollset_updater(pipe_socket read_handle, multiplexer* parent);

  // -- interface functions ----------------------------------------------------

  bool handle_read_event() override;

  bool handle_write_event() override;

private:
  msg_buf buf_;
  size_t buf_size_ = 0;
};

} // namespace net