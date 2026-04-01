/**
 *  @author    Jakob Otto
 *  @file      timeout_entry.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/socket_id.hpp"

#include <chrono>
#include <cstdint>

namespace net {

struct timeout_entry {
  timeout_entry(socket_id handle, std::chrono::steady_clock::time_point when,
                uint64_t id)
    : handle_{handle}, when_{when}, id_{id} {
    // nop
  }

  // -- Comparison operations --------------------------------------------------

  bool operator<=>(const timeout_entry& other) const = default;

  bool operator<(const timeout_entry& other) const {
    return when_ < other.when_;
  }

  bool operator>(const timeout_entry& other) const {
    return when_ > other.when_;
  }

  bool operator==(const timeout_entry& other) const = default;

  // -- members ----------------------------------------------------------------

  socket_id handle_;
  std::chrono::steady_clock::time_point when_;
  uint64_t id_;
};

} // namespace net
