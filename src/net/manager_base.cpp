/**
 *  @author    Jakob Otto
 *  @file      manager_base.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/manager_base.hpp"

#include "net/event_result.hpp"
#include "net/multiplexer_base.hpp"

#include "util/error.hpp"
#include "util/logger.hpp"

#include <iostream>
#include <memory>
#include <sys/socket.h>

namespace net {

manager_base::manager_base(socket handle, multiplexer_base* mpx)
  : handle_{handle}, mpx_{mpx} {
  LOG_TRACE();
}

/// Initializes a manager_base
util::error manager_base::init(const util::config&) {
  return util::none;
}

manager_base::~manager_base() {
  LOG_TRACE();
  shutdown(handle_, SHUT_RDWR);
  close(handle_);
}

manager_base::manager_base(manager_base&& other) noexcept
  : handle_{other.handle_}, mpx_{other.mpx_}, mask_{other.mask_} {
  LOG_TRACE();
  other.handle_ = invalid_socket;
}

manager_base& manager_base::operator=(manager_base&& other) noexcept {
  LOG_TRACE();
  handle_ = other.handle_;
  other.handle_ = invalid_socket;
  mpx_ = other.mpx_;
  mask_ = other.mask_;
  return *this;
}

bool manager_base::mask_add(operation flag) noexcept {
  if ((mask() & flag) == flag) {
    return false;
  }
  mask_set(mask() | flag);
  return true;
}

bool manager_base::mask_del(operation flag) noexcept {
  if ((mask() & flag) == operation::none) {
    return false;
  }
  mask_set(mask() & ~flag);
  return true;
}

void manager_base::register_reading() {
  if ((mask() & operation::read) == operation::none) {
    mpx()->enable(*this, operation::read);
  }
}

void manager_base::register_writing() {
  if ((mask() & operation::write) == operation::none) {
    mpx()->enable(*this, operation::write);
  }
}

event_result manager_base::handle_read_event() {
  LOG_ERROR("Default implementation, should never be called");
  return event_result::error;
}

event_result manager_base::handle_write_event() {
  LOG_ERROR("Default implementation, should never be called");
  return event_result::error;
}

uint64_t manager_base::set_timeout_in(std::chrono::steady_clock::duration in) {
  const auto when = std::chrono::steady_clock::now() + in;
  return set_timeout_at(when);
}

uint64_t
manager_base::set_timeout_at(std::chrono::steady_clock::time_point when) {
  return mpx()->set_timeout(this, when);
}

event_result manager_base::handle_timeout(uint64_t) {
  LOG_ERROR("Default implementation, should never be called");
  return event_result::error;
}

} // namespace net
