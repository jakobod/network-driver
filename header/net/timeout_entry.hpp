
/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <chrono>
#include <cstddef>

#include "net/socket_id.hpp"

namespace net {

struct timeout_entry {
  timeout_entry(socket_id handle, std::chrono::system_clock::time_point when,
                uint64_t id)
    : handle_{handle}, when_{when}, id_{id} {
  }

  socket_id handle_;
  std::chrono::system_clock::time_point when_;
  uint64_t id_;

  bool operator<(const timeout_entry& other) const {
    return when_ < other.when_;
  }

  bool operator>(const timeout_entry& other) const {
    return when_ > other.when_;
  }

  bool operator==(const timeout_entry& other) const {
    return (handle_ == other.handle_) && (when_ == other.when_)
           && (id_ == other.id_);
  }
};

} // namespace net
