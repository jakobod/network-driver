/**
 *  @author    Jakob Otto
 *  @file      operation.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/event_result.hpp"

#include <iostream>

namespace net {

std::ostream& operator<<(std::ostream& os, event_result op) {
  return os << to_string(op);
}

} // namespace net
