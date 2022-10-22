/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <gtest/gtest.h>

#include <iostream>

// -- Helper macros for tests --------------------------------------------------

#define MESSAGE()                                                              \
  std::cerr << "\033[0;33m"                                                    \
            << "[   info   ] "                                                 \
            << "\033[m"

#define EXPECT_NO_ERROR(expr)                                                  \
  do {                                                                         \
    auto err = expr;                                                           \
    EXPECT_FALSE(err.is_error())                                               \
      << #expr << " returned an error: " << err << std::endl;                  \
  } while (false)

#define ASSERT_NO_ERROR(expr)                                                  \
  do {                                                                         \
    if (auto err = (expr))                                                     \
      FAIL() << #expr << " returned an error: " << err << std::endl;           \
  } while (false)
