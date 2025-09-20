/**
 *  @author    Jakob Otto
 *  @file      event_result.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <cstdint>
#include <iostream>
#include <string_view>

namespace net {

/// Event results after handling events
enum class event_result : std::uint8_t {
  ok = 0,
  done,
  error,
};

static constexpr std::string_view to_string(event_result op) noexcept {
  switch (op) {
    case event_result::ok:
      return "event_result::ok";
    case event_result::done:
      return "event_result::done";
    case event_result::error:
      return "event_result::error";
    default:
      return "event_result::unknown";
  }
}

std::ostream& operator<<(std::ostream& os, event_result op);

} // namespace net
