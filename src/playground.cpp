/**
 *  @author    Jakob Otto
 *  @file      playground.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "meta/concepts.hpp"

#include "util/config.hpp"
#include "util/logger.hpp"

#include <iostream>

using namespace std::string_literals;

int main(int, const char**) {
  util::config cfg;
  try {
    cfg.parse("config.cfg");
  } catch (const std::runtime_error& err) {
    LOG_ERROR(err.what());
  }

  LOG_INIT(cfg);

  LOG_TRACE();
  LOG_ERROR("error");
  LOG_WARNING("warning");
  LOG_DEBUG("debug");
}
