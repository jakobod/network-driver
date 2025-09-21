/**
 *  @author    Jakob Otto
 *  @file      operation.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/manager.hpp"

#include "net/application.hpp"
#include "net/multiplexer.hpp"
#include "net/receive_policy.hpp"

#include "net/sockets/socket.hpp"

#include "util/logger.hpp"

namespace net {

/// Constructs a manager object
manager::manager(sockets::socket handle, multiplexer* mpx)
  : handle_(handle), mpx_(mpx), mask_(operation::none) {
  LOG_TRACE();
}

/// Destructs a socket manager object
manager::~manager() {
  LOG_TRACE();
  close(handle_);
}

/// Move constructor
manager::manager(manager&& other) noexcept
  : mpx_(other.mpx_), mask_(other.mask_) {
  LOG_TRACE();
  std::swap(other.handle_, handle_);
}

/// Move assignment operator
manager& manager::operator=(manager&& other) noexcept {
  LOG_TRACE();
  std::swap(handle_, other.handle_);
  mask_ = other.mask_;
  mpx_  = other.mpx_;
  return *this;
}

// -- properties -------------------------------------------------------------

util::byte_buffer& manager::write_buffer() noexcept {
  return write_buffer_;
}

// -- Operation registration -------------------------------------------------

/// Returns registered operations (read, write, or both).
operation manager::mask() const noexcept {
  return mask_;
}

/// Sets the event mask to the given flag(s).
void manager::mask_set(operation flag) noexcept {
  mask_ = flag;
}

/// Adds given flag(s) to the event mask.
bool manager::mask_add(operation flag) noexcept {
  if ((mask() & flag) == flag) {
    return false;
  }
  mask_set(mask() | flag);
  return true;
}

/// Tries to clear given flag(s) from the event mask.
bool manager::mask_del(operation flag) noexcept {
  if ((mask() & flag) == operation::none) {
    return false;
  }
  mask_set(mask() & ~flag);
  return true;
}

// -- Timeout handling -------------------------------------------------------

/// Sets a timeout in `duration` milliseconds with the id `timeout_id`
uint64_t manager::set_timeout_in(std::chrono::milliseconds duration) {
  const auto tp = std::chrono::steady_clock::now() + duration;
  return set_timeout_at(tp);
}

/// Sets a timeout at timepoint `point` with the id `timeout_id`
uint64_t
manager::set_timeout_at(std::chrono::steady_clock::time_point timepoint) {
  return mpx()->set_timeout(this, timepoint);
  return 0;
}

} // namespace net
