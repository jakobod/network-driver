/**
 *  @author    Jakob Otto
 *  @file      manager_base.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "net/event_result.hpp"

#include "net/detail/manager_base.hpp"

#include "util/logger.hpp"

namespace net::detail {

class event_handler : public manager_base {
public:
  event_handler(net::socket handle, multiplexer_base* mpx)
    : manager_base{handle, mpx} {
    LOG_TRACE();
    nonblocking(handle, true);
  }

  // -- Event handling ---------------------------------------------------------

  virtual event_result handle_read_event() { return event_result::error; }

  virtual event_result handle_write_event() { return event_result::error; }
};

using event_handler_ptr = util::intrusive_ptr<event_handler>;

} // namespace net::detail
