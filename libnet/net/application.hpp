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

#include <memory>

namespace net {

/// Manages the lifetime of a socket and its events.
struct application {
  virtual ~application() = default;

  virtual util::error init(const util::config& cfg) = 0;

  virtual util::error consume(util::const_byte_span data) = 0;

  virtual void handle_timeout(std::uint64_t timeout_id) = 0;

  manager* mgr() noexcept { return mgr_; }

private:
  manager* mgr_{nullptr};
};

using application_ptr = std::unique_ptr<application>;

} // namespace net
