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

namespace net::detail {

class event_handler : public manager_base {
public:
  // -- Constructors, destructors ----------------------------------------------

  using manager_base::manager_base;

  virtual ~event_handler() = default;

  // -- Move and assignment operations -----------------------------------------

  event_handler(const event_handler&) = default;
  event_handler(event_handler&& mgr) noexcept = default;
  event_handler& operator=(const event_handler&) = default;
  event_handler& operator=(event_handler&& other) = default;

  // -- Event handling ---------------------------------------------------------

  virtual event_result handle_read_event() { return event_result::error; }

  virtual event_result handle_write_event() { return event_result::error; }
};

using event_handler_ptr = util::intrusive_ptr<event_handler>;

} // namespace net::detail
