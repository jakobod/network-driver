/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/event_result.hpp"

namespace net {

std::string to_string(event_result res) {
  switch (res) {
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

} // namespace net
