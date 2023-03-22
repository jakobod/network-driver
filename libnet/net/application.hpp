/**
 *  @author    Jakob Otto
 *  @file      application.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

namespace net {

struct application {
  /// Initializes the application.
  virtual util::error init(const util::config& cfg) = 0;

  /// Checks wether the application has more data to send.
  virtual bool has_more_data() = 0;

  /// Produces more data and enqueues it at their parent.
  virtual event_result produce() = 0;

  /// Consumes previously received data.
  virtual event_result consume(util::const_byte_span bytes) = 0;

  /// Handles a timeout.
  virtual event_result handle_timeout(uint64_t id) = 0;
};

} // namespace net
