/**
 *  @author    Jakob Otto
 *  @file      manager.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/uring/manager.hpp"

namespace net::uring {

/// Constructs a uring_manager object
manager::manager(sockets::socket handle, multiplexer_impl* mpx)
  : handle_{handle}, mpx_{mpx} {}

/// Destructs a socket manager object
manager::~manager() {
  close(handle_);
}

void manager::mask_set(operation flag) noexcept {
  mask_ = flag;
}

bool manager::mask_add(operation flag) noexcept {
  if ((mask() & flag) == flag) {
    return false;
  }
  mask_set(mask() | flag);
  return true;
}

bool manager::mask_del(operation flag) noexcept {
  if ((mask() & flag) == operation::none) {
    return false;
  }
  mask_set(mask() & ~flag);
  return true;
}

} // namespace net::uring
