/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/pollset_updater.hpp"

#include "net/event_result.hpp"
#include "net/multiplexer.hpp"
#include "net/pipe_socket.hpp"

#include "util/error.hpp"
#include "util/logger.hpp"

namespace {

template <class T>
util::error read_from_pipe(net::pipe_socket handle, T& t) {
  if (auto res = net::read(handle, util::as_bytes(&t, sizeof(T)));
      res != sizeof(T)) {
    LOG_ERROR("Could not read ", sizeof(T),
              " bytes from pipe socket: ", net::last_socket_error_as_string());
    return util::error_code::runtime_error;
  }
  return util::none;
}

} // namespace

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
  if (auto err = read_from_pipe(handle<pipe_socket>(), code))
    return event_result::ok;
  switch (code) {
    case add_code: {
      socket_manager* mgr_ptr = nullptr;
      operation op;
      if (auto err = read_from_pipe(handle<pipe_socket>(), mgr_ptr))
        return event_result::ok;
      if (auto err = read_from_pipe(handle<pipe_socket>(), op))
        return event_result::ok;
      LOG_DEBUG("Received add_code for mgr with ",
                NET_ARG2("id", mgr_ptr->handle().id), " with ", NET_ARG(op));
      mpx()->add(util::make_intrusive(mgr_ptr, false), op);
      break;
    }
    case shutdown_code:
      LOG_DEBUG("Received shutdown_code");
      mpx()->shutdown();
      return event_result::done;
    default:
      LOG_WARNING("Received unspecified code");
      break;
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
