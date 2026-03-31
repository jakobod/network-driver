/**
 *  @author    Jakob Otto
 *  @file      multiplexer_base.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "net/manager_base.hpp"
#include "net/operation.hpp"

namespace net {

class multiplexer_base {
public:
  virtual ~multiplexer_base() = default;

  /// Handles an error `err`.
  virtual void handle_error(const util::error& err) = 0;

  /// Adds a new fd to the multiplexer_base for operation `initial`.
  /// @warn This function is *NOT* thread-safe.
  virtual void add(manager_base_ptr mgr, operation initial) = 0;

  virtual void shutdown() = 0;
};

} // namespace net
