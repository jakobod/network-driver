/**
 *  @author    Jakob Otto
 *  @file      scope_guard.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "util/scope_guard.hpp"

#include "net_test.hpp"

TEST(scope_guard, func_executed) {
  bool executed = false;
  {
    const util::scope_guard guard([&]() { executed = true; });
  }
  EXPECT_TRUE(executed);
}

TEST(scope_guard, func_not_executed) {
  bool executed = false;
  const util::scope_guard guard([&]() { executed = true; });
  EXPECT_FALSE(executed);
}

TEST(scope_guard, reset) {
  bool executed = false;
  {
    util::scope_guard guard([&]() { executed = true; });
    guard.reset();
  }
  EXPECT_FALSE(executed);
}
