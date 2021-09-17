/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <chrono>

#include "fwd.hpp"
#include "net/socket_id.hpp"

namespace net {

static const size_t max_epoll_events = 32;

struct timeout_entry {
  socket_id hdl;
  std::chrono::system_clock::time_point when;
  uint64_t id;

  bool operator<(const timeout_entry& other) const {
    return when < other.when;
  }

  bool operator>(const timeout_entry& other) const {
    return when > other.when;
  }

  bool operator==(const timeout_entry& other) const {
    return (hdl == other.hdl) && (when == other.when) && (id == other.id);
  }
};

} // namespace net
