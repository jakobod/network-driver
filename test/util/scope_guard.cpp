/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include <gtest/gtest.h>

#include "util/scope_guard.hpp"

using namespace net;

TEST(scope_guard_test, func_executed) {
  bool executed = false;
  {
    util::scope_guard guard([&]() { executed = true; });
  }
  EXPECT_TRUE(executed);
}

TEST(scope_guard_test, func_not_executed) {
  bool executed = false;
  util::scope_guard guard([&]() { executed = true; });
  EXPECT_FALSE(executed);
}
