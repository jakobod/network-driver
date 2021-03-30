/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 30.03.2021
 *
 *  This file is based on `socket_manager.cpp` from the C++ Actor Framework.
 *  https://github.com/actor-framework/incubator
 */

#pragma once

#include "net/socket_manager_impl.hpp"

#include <memory>

namespace net {

socket_manager_impl::socket_manager_impl() {
  // nop
}

bool socket_manager_impl::handle_read_event() {
  // TODO: read from the socket
  return false;
}

bool socket_manager_impl::handle_write_event() {
  // TODO: write from the socket
  return false;
}

} // namespace net
