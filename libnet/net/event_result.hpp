/**
 *  @author    Jakob Otto
 *  @file      event_result.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <cstdint>
#include <string>

namespace net {

/// Event results after handling events
enum class event_result : std::uint8_t {
  ok = 0,
  done,
  error,
};

std::string to_string(event_result op);

} // namespace net
