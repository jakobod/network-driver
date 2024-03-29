/**
 *  @author    Jakob Otto
 *  @file      net_test_main.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net_test.hpp"

#include "util/config.hpp"
#include "util/logger.hpp"

int main(int argc, char** argv) {
  // Disables logging alltogether
  util::config cfg;
  cfg.add_config_entry("logger.terminal-output", false);
  LOG_INIT(cfg);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
