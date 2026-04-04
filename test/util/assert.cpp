/**
 *  @author    Jakob Otto
 *  @file      assert.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net_test.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

std::stringstream test_stream;
void test_abort();
#define ABORT_FUNCTION test_abort
#define ASSERTION_OUTPUT_STREAM test_stream
#include "util/assert.hpp"

// Custom exception for testing assertion failures
struct AssertionFailed : public std::exception {
  explicit AssertionFailed(std::string message)
    : message_{std::move(message)} {}

  const char* what() const noexcept override { return message_.c_str(); }

private:
  std::string message_;
};

// Custom abort for testing - throws instead of calling std::abort()
// This allows assertions to be caught and tested without terminating the
// program
void test_abort() {
  throw AssertionFailed{"Assertion Failed"};
}

} // namespace

#if defined(LIB_NET_ENABLE_ASSERTIONS)

class AssertTest : public ::testing::Test {
protected:
  void TearDown() override {
    // Clear the stream after each test
    test_stream.str("");
    test_stream.clear();
  }
};

TEST_F(AssertTest, true_condition_passes) {
  // True condition should not throw
  EXPECT_NO_THROW(ASSERT(5 > 3));
  EXPECT_TRUE(test_stream.str().empty());
}

TEST_F(AssertTest, false_condition_fails) {
  // False condition should throw AssertionFailed via custom abort()
  EXPECT_THROW(ASSERT(1 > 2), AssertionFailed);
  // Verify that error message was output
  const auto output = test_stream.str();
  EXPECT_NE(output.find("ASSERTION FAILED"), std::string::npos);
  EXPECT_NE(output.find("1 > 2"), std::string::npos);
}

TEST_F(AssertTest, assert_with_streamed_message) {
  // Assertion with streamed message should throw and capture message
  EXPECT_THROW(ASSERT(false) << "custom error message", AssertionFailed);

  // Verify that both condition and message were output
  const auto output = test_stream.str();
  EXPECT_NE(output.find("ASSERTION FAILED"), std::string::npos);
  EXPECT_NE(output.find("custom error message"), std::string::npos);
}

TEST_F(AssertTest, assert_with_multiple_streamed_values) {
  const int value = 42;
  // Multiple values streamed to assertion message
  EXPECT_THROW(ASSERT(value < 10) << "value=" << value << " is too large",
               AssertionFailed);

  const auto output = test_stream.str();
  EXPECT_NE(output.find("value=42"), std::string::npos);
  EXPECT_NE(output.find("is too large"), std::string::npos);
}

TEST_F(AssertTest, debug_only_assert_in_debug_build) {
#  if not defined(LIB_NET_DEBUG)
  // Skip in non-debug builds
  GTEST_SKIP() << "LIB_NET_DEBUG not defined";
#  else
  // Debug assertions are enabled in debug builds
  EXPECT_NO_THROW(DEBUG_ONLY_ASSERT(3 < 5));
  EXPECT_TRUE(test_stream.str().empty());

  // False debug assertion should throw
  EXPECT_THROW(DEBUG_ONLY_ASSERT(1 > 2), AssertionFailed);
#  endif
}

#else // LIB_NET_ENABLE_ASSERTIONS not defined

TEST(AssertTest, assert_disabled_noop) {
  // When assertions are disabled, they should be no-ops
  EXPECT_NO_THROW(ASSERT(false));            // Should not do anything
  EXPECT_NO_THROW(DEBUG_ONLY_ASSERT(false)); // Should not do anything
}

#endif
