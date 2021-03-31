/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 29.03.2021
 *
 *  This file is based on `socket_manager.cpp` from the C++ Actor Framework.
 *  https://github.com/actor-framework/incubator
 */

#pragma once

#include "net/socket_manager.hpp"

#include <memory>

#include "net/multiplexer.hpp"

namespace net {

socket_manager::socket_manager(socket handle, multiplexer* mpx)
  : handle_(handle), mask_(operation::none), mpx_(mpx) {
  // nop
}

socket_manager::~socket_manager() {
  close(handle_);
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

void socket_manager::register_reading() {
  if ((mask() & operation::read) == operation::read)
    return;
  mpx_->register_reading(handle());
}

void socket_manager::register_writing() {
  if ((mask() & operation::write) == operation::write)
    return;
  mpx_->register_writing(handle());
}

bool handle_read_event() {
  // TODO: read from the socket
  return false;
}

bool handle_write_event() {
  // TODO: write from the socket
  return false;
}

} // namespace net
