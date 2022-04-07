/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <gtest/gtest.h>

#define EXPECT_NO_ERROR(expr)                                                  \
  if (auto err = expr)                                                         \
    FAIL() << #expr << " returned an error: " << err << std::endl;
