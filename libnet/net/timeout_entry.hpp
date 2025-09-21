/**
 *  @author    Jakob Otto
 *  @file      timeout_entry.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/sockets/socket_id.hpp"

#include <chrono>
#include <cstdint>

namespace net {

struct timeout_entry {
  constexpr bool operator<(const timeout_entry& other) const noexcept {
    return when_ < other.when_;
  }

  constexpr bool operator>(const timeout_entry& other) const noexcept {
    return when_ > other.when_;
  }

  constexpr bool operator==(const timeout_entry& other) const noexcept
    = default;

  sockets::socket_id handle_;
  std::chrono::steady_clock::time_point when_;
  uint64_t id_;
};

} // namespace net
