/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/pollset_updater.hpp"

#include "net/event_result.hpp"
#include "net/multiplexer.hpp"
#include "net/pipe_socket.hpp"

#include "util/byte_span.hpp"
#include "util/error.hpp"

#include <cstddef>

namespace net {

pollset_updater::pollset_updater(pipe_socket handle, multiplexer* mpx)
  : socket_manager(handle, mpx) {
  // nop
}

util::error pollset_updater::init() {
  return util::none;
}

// -- interface functions ----------------------------------------------------

event_result pollset_updater::handle_read_event() {
  opcode code;
  auto res = read(handle<pipe_socket>(), util::as_bytes(&code, 1));
  if (res > 0) {
    switch (code) {
      case add_code:
      case enable_code:
      case disable_code:
        /// Currently not implemented
        break;
      case shutdown_code:
        mpx()->shutdown();
        return event_result::done;
      default:
        break;
    }
  }
  return event_result::ok;
}

event_result pollset_updater::handle_write_event() {
  mpx()->handle_error(util::error(
    util::error_code::runtime_error,
    "[pollset_updater::handle_write_event()] pollset_updater should not "
    "be registered for writing"));
  return event_result::error;
}

event_result pollset_updater::handle_timeout(uint64_t) {
  mpx()->handle_error(
    util::error(util::error_code::runtime_error,
                "[pollset_updater::handle_timeout()] not implemented!"));
  return event_result::error;
}

} // namespace net
