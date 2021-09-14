/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/pollset_updater.hpp"

#include <cstddef>
#include <iostream>
#include <span>

#include "net/error.hpp"
#include "net/multiplexer.hpp"
#include "net/pipe_socket.hpp"

namespace net {

pollset_updater::pollset_updater(pipe_socket handle, multiplexer* mpx)
  : socket_manager(handle, mpx) {
  // nop
}

// -- interface functions ----------------------------------------------------

bool pollset_updater::handle_read_event() {
  opcode code;
  auto res = read(handle<pipe_socket>(),
                  std::span{reinterpret_cast<std::byte*>(&code), 1});
  if (res > 0) {
    switch (code) {
      case add_code:
        break;
      case enable_code:
        break;
      case disable_code:
        break;
      case shutdown_code:
        mpx()->shutdown();
        return false;
      default:
        break;
    }
  }
  return true;
}

bool pollset_updater::handle_write_event() {
  mpx()->handle_error(
    error(runtime_error,
          "[pollset_updater::handle_write_event()] pollset_updater should not "
          "be registered for writing"));
  return false;
}

bool pollset_updater::handle_timeout(uint64_t) {
  mpx()->handle_error(error(
    runtime_error, "[pollset_updater::handle_timeout()] not implemented!"));
  return false;
}

} // namespace net
