/**
 *  @author    Jakob Otto
 *  @file      timeout_entry.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/socket/socket_id.hpp"

#include <chrono>
#include <cstdint>

namespace net {

struct timeout_entry {
  using timeout_id = uint64_t;

  timeout_entry(socket_id handle, std::chrono::steady_clock::time_point when,
                timeout_id id)
    : handle_{handle}, when_{when}, id_{id} {
    // nop
  }

  // -- properties -------------------------------------------------------------

  socket_id handle() const noexcept { return handle_; }

  std::chrono::steady_clock::time_point when() const noexcept { return when_; }

  timeout_id id() const noexcept { return id_; }

  bool has_expired() const noexcept {
    return when_ <= std::chrono::steady_clock::now();
  }

  // -- Comparison operations --------------------------------------------------

  bool operator<(const timeout_entry& other) const noexcept {
    return when_ < other.when_;
  }

  bool operator>(const timeout_entry& other) const noexcept {
    return when_ > other.when_;
  }

  bool operator==(const timeout_entry& other) const noexcept = default;

private:
  // -- members ----------------------------------------------------------------

  socket_id
    handle_; // TODO: Add a pointer to the manager that should be triggered
  std::chrono::steady_clock::time_point when_;
  timeout_id id_;
};

} // namespace net
