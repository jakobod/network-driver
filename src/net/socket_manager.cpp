/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/socket_manager.hpp"

#include "net/multiplexer.hpp"

#include "util/error.hpp"
#include "util/logger.hpp"

#include <iostream>
#include <memory>

namespace net {

socket_manager::socket_manager(socket handle, multiplexer* mpx)
  : handle_(handle), mpx_(mpx), mask_(operation::none) {
  LOG_TRACE();
}

socket_manager::~socket_manager() {
  LOG_TRACE();
  shutdown(handle_, SHUT_RDWR);
  close(handle_);
}

socket_manager::socket_manager(socket_manager&& other) noexcept
  : handle_(other.handle_), mpx_(other.mpx_), mask_(other.mask_) {
  LOG_TRACE();
  other.handle_ = invalid_socket;
}

socket_manager& socket_manager::operator=(socket_manager&& other) noexcept {
  LOG_TRACE();
  handle_ = other.handle_;
  other.handle_ = invalid_socket;
  mask_ = other.mask_;
  mpx_ = other.mpx_;
  return *this;
}

void socket_manager::mask_set(operation flag) noexcept {
  mask_ = flag;
}

bool socket_manager::mask_add(operation flag) noexcept {
  if ((mask() & flag) == flag)
    return false;
  mask_set(mask() | flag);
  return true;
}

bool socket_manager::mask_del(operation flag) noexcept {
  if ((mask() & flag) == operation::none)
    return false;
  mask_set(mask() & ~flag);
  return true;
}

void socket_manager::register_reading() {
  LOG_TRACE();
  if ((mask() & operation::read) == operation::read)
    return;
  mpx_->enable(*this, operation::read);
}

void socket_manager::register_writing() {
  LOG_TRACE();
  if ((mask() & operation::write) == operation::write)
    return;
  mpx_->enable(*this, operation::write);
}

uint64_t socket_manager::set_timeout_in(std::chrono::milliseconds duration) {
  auto tp = std::chrono::system_clock::now() + duration;
  return mpx_->set_timeout(*this, tp);
}

uint64_t
socket_manager::set_timeout_at(std::chrono::system_clock::time_point point) {
  return mpx_->set_timeout(*this, point);
}

void socket_manager::handle_error(const util::error& err) {
  mpx_->handle_error(err);
}

} // namespace net
