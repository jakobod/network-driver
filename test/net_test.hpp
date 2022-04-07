/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <gtest/gtest.h>

#define EXPECT_NO_ERROR(expr)                                                  \
  do {                                                                         \
    auto err = expr;                                                           \
    EXPECT_FALSE(err.is_error())                                               \
      << #expr << " returned an error: " << err << std::endl;                  \
  } while (false)

#define ASSERT_NO_ERROR(expr)                                                  \
  do {                                                                         \
    if (auto err = expr)                                                       \
      FAIL() << #expr << " returned an error: " << err << std::endl;           \
  } while (false)
