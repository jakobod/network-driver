/**
 *  @author    Jakob Otto
 *  @file      multiplexer_base.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/multiplexer.hpp"

#include "util/error_or.hpp"
#include "util/logger.hpp"

namespace net {

util::error_or<multiplexer_ptr> make_multiplexer(manager_factory factory,
                                                 const util::config& cfg) {
  LOG_TRACE();
  auto mpx = std::make_shared<multiplexer>();
  if (auto err = mpx->init(std::move(factory), cfg)) {
    return err;
  }
  return mpx;
}

} // namespace net
