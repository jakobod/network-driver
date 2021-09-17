/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <string>

namespace net {

/// Event results after handling events
enum class event_result : uint8_t {
  ok = 0,
  done,
  error,
};

std::string to_string(event_result op);

} // namespace net
