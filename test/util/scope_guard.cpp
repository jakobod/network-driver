/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include <gtest/gtest.h>

#include "util/scope_guard.hpp"

TEST(scope_guard, func_executed) {
  bool executed = false;
  {
    util::scope_guard guard([&]() { executed = true; });
  }
  EXPECT_TRUE(executed);
}

TEST(scope_guard, func_not_executed) {
  bool executed = false;
  util::scope_guard guard([&]() { executed = true; });
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
