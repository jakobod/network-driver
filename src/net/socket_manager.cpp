/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 29.03.2021
 *
 *  This file is based on `socket_manager.cpp` from the C++ Actor Framework.
 *  https://github.com/actor-framework/incubator
 */

#include "net/socket_manager.hpp"

#include <iostream>
#include <memory>

#include "net/error.hpp"
#include "net/multiplexer.hpp"

namespace net {

socket_manager::socket_manager(socket handle, multiplexer* mpx)
  : handle_(handle), mask_(operation::none), mpx_(mpx) {
  // nop
}

socket_manager::~socket_manager() {
  shutdown(handle_, SHUT_RDWR);
  close(handle_);
}

socket_manager::socket_manager(socket_manager&& other)
  : handle_(other.handle_), mask_(other.mask_), mpx_(other.mpx_) {
  other.handle_ = invalid_socket;
}

socket_manager& socket_manager::operator=(socket_manager&& other) {
  handle_ = other.handle_;
  other.handle_ = invalid_socket;
  mask_ = other.mask_;
  mpx_ = other.mpx_;
  return *this;
}

bool socket_manager::mask_add(operation flag) noexcept {
  auto x = mask();
  if ((x & flag) == flag)
    return false;
  mask_ = x | flag;
  return true;
}

bool socket_manager::mask_del(operation flag) noexcept {
  auto x = mask();
  if ((x & flag) == operation::none)
    return false;
  mask_ = x & ~flag;
  return true;
}

void socket_manager::register_writing() {
  if ((mask() & operation::write) == operation::write)
    return;
  mpx_->enable(*this, operation::write);
}

void socket_manager::set_timeout_in(std::chrono::milliseconds duration,
                                    uint64_t timeout_id) {
  auto tp = std::chrono::system_clock::now() + duration;
  mpx()->set_timeout(*this, timeout_id, tp);
}

void socket_manager::set_timeout_at(std::chrono::system_clock::time_point point,
                                    uint64_t timeout_id) {
  mpx()->set_timeout(*this, timeout_id, point);
}

void socket_manager::handle_error(error err) {
  mpx()->handle_error(err);
}

} // namespace net
