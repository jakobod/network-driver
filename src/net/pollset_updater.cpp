/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 07.04.2021
 */

#include "net/pollset_updater.hpp"

#include <iostream>

#include "net/multiplexer.hpp"
#include "net/pipe_socket.hpp"

namespace net {

pollset_updater::pollset_updater(pipe_socket handle, multiplexer* mpx)
  : socket_manager(handle, mpx) {
  std::cout << "pollset_updater got fd = " << handle.id << std::endl;
  // nop
}

// -- interface functions ----------------------------------------------------

bool pollset_updater::handle_read_event() {
  std::cout << "pollset_updater read event!" << std::endl;
  msg_buf buf;
  auto res = read(handle<pipe_socket>(), buf);
  if (res > 0) {
    std::cout << "read " << res << " bytes from the pipe_socket" << std::endl;
    if (static_cast<opcode>(buf.front()) == shutdown_code) {
      mpx()->shutdown();
      return false;
    }
  }
  return true;
}

bool pollset_updater::handle_write_event() {
  mpx()->handle_error(
    detail::error(detail::runtime_error,
                  "pollset_updater should not be registered for reading!"));
  return false;
}

} // namespace net
