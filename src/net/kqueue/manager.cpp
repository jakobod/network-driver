/**
 *  @author    Jakob Otto
 *  @file      manager.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/kqueue/manager.hpp"

#include "net/kqueue/multiplexer.hpp"

#include "net/event_result.hpp"

#include "util/error.hpp"
#include "util/logger.hpp"

#include <iostream>
#include <memory>

#include <sys/socket.h>

namespace net::kqueue {

manager::manager(socket handle, multiplexer_base* mpx)
  : manager_base{handle, mpx} {
  LOG_TRACE();
}

void manager::register_reading() {
  LOG_TRACE();
  if ((mask() & operation::read) == operation::read) {
    return;
  }
  mpx<multiplexer>()->enable(*this, operation::read);
}

void manager::register_writing() {
  LOG_TRACE();
  if ((mask() & operation::write) == operation::write) {
    return;
  }
  mpx<multiplexer>()->enable(*this, operation::write);
}

uint64_t manager::set_timeout_in(std::chrono::milliseconds duration) {
  const auto tp = std::chrono::system_clock::now() + duration;
  return mpx<multiplexer>()->set_timeout(this, tp);
}

uint64_t manager::set_timeout_at(std::chrono::system_clock::time_point point) {
  return mpx<multiplexer>()->set_timeout(this, point);
}

/// Handles a read-event
event_result manager::handle_read_event() {
  LOG_ERROR("This function is merely a default implementation.");
  return event_result::error;
}

/// Handles a write-event
event_result manager::handle_write_event() {
  LOG_ERROR("This function is merely a default implementation.");
  return event_result::error;
}

/// Handles a timeout-event
event_result manager::handle_timeout(uint64_t) {
  LOG_ERROR("This function is merely a default implementation.");
  return event_result::error;
}

void manager::handle_error(const util::error& err) {
  mpx<multiplexer>()->handle_error(err);
}

} // namespace net::kqueue
