/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/event_result.hpp"

namespace net {

std::string to_string(event_result res) {
  switch (res) {
    case event_result::ok:
      return "ok";
    case event_result::done:
      return "done";
    case event_result::error:
      return "error";
    default:
      return "unknown result";
  }
}

} // namespace net
