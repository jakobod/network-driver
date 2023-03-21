/**
 *  @author    Jakob Otto
 *  @file      event_result.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
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
