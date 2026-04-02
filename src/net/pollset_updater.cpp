/**
 *  @author    Jakob Otto
 *  @file      pollset_updater.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/detail/pollset_updater.hpp"

#include "net/detail/multiplexer_base.hpp"

#include "net/event_result.hpp"
#include "net/socket/pipe_socket.hpp"

#include "util/byte_span.hpp"
#include "util/error.hpp"
#include "util/logger.hpp"

namespace {

template <class T>
util::error read_from_pipe(net::pipe_socket handle, T& t) {
  if (const auto res = net::read(handle, util::as_bytes(&t, sizeof(T)));
      res != sizeof(T)) {
    LOG_ERROR("Could not read ", sizeof(T),
              " bytes from pipe socket: ", net::last_socket_error_as_string());
    return util::error_code::runtime_error;
  }
  return util::none;
}

} // namespace

namespace net::detail {

pollset_updater::pollset_updater(pipe_socket handle, multiplexer_base* mpx)
  : event_handler{handle, mpx} {
  LOG_TRACE();
}

// -- interface functions ----------------------------------------------------

event_result pollset_updater::handle_read_event() {
  LOG_TRACE();
  opcode code = opcode::none;
  if (auto err = read_from_pipe(handle<pipe_socket>(), code)) {
    return event_result::ok;
  }
  switch (code) {
    case opcode::add: {
      event_handler* mgr_ptr = nullptr;
      operation op;
      if (auto err = read_from_pipe(handle<pipe_socket>(), mgr_ptr)) {
        return event_result::ok;
      }
      if (auto err = read_from_pipe(handle<pipe_socket>(), op)) {
        return event_result::ok;
      }
      LOG_DEBUG("Received add_code for mgr with ",
                NET_ARG2("id", mgr_ptr->handle().id), " with ", NET_ARG(op));
      mpx()->add(util::make_intrusive(mgr_ptr, false), op);
      break;
    }
    case opcode::shutdown:
      LOG_DEBUG("Received shutdown_code");
      mpx()->shutdown();
      return event_result::done;
    default:
      LOG_WARNING("Received unspecified code");
      break;
  }
  return event_result::ok;
}

} // namespace net::detail
