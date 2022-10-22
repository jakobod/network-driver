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
#include "util/logger.hpp"

namespace net {

pollset_updater::pollset_updater(pipe_socket handle, multiplexer* mpx)
  : socket_manager(handle, mpx) {
  LOG_TRACE();
}

util::error pollset_updater::init() {
  LOG_TRACE();
  return util::none;
}

// -- interface functions ----------------------------------------------------

event_result pollset_updater::handle_read_event() {
  LOG_TRACE();
  opcode code;
  const auto res = read(handle<pipe_socket>(), util::as_bytes(&code, 1));
  if (res > 0) {
    switch (code) {
      case add_code:
        LOG_DEBUG("Received add_code");
        break;
      case enable_code:
        LOG_DEBUG("Received enable_code");
        break;
      case disable_code:
        LOG_DEBUG("Received disable_code");
        break;
      case shutdown_code:
        LOG_DEBUG("Received shutdown_code");
        mpx()->shutdown();
        return event_result::done;
      default:
        LOG_WARNING("Received unspecified code");
        break;
    }
  }
  return event_result::ok;
}

event_result pollset_updater::handle_write_event() {
  LOG_ERROR("Should not be registered for write_events");
  mpx()->handle_error(
    {util::error_code::runtime_error,
     "[pollset_updater::handle_write_event()] pollset_updater should not "
     "be registered for writing"});
  return event_result::error;
}

event_result pollset_updater::handle_timeout(uint64_t) {
  LOG_ERROR("Should not be registered for timeouts");
  mpx()->handle_error({util::error_code::runtime_error,
                       "[pollset_updater::handle_timeout()] not implemented!"});
  return event_result::error;
}

} // namespace net
