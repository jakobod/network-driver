/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include <gtest/gtest.h>

#include "util/format.hpp"

#include <algorithm>

TEST(format_test, replace_single_string) {
  auto res = util::format("Hello {0}", "World");
  ASSERT_EQ("Hello World", res);
}

TEST(format_test, replace_multiple_strings) {
  auto res = util::format("{0} {1}", "Hello", "World");
  ASSERT_EQ("Hello World", res);
}

TEST(format_test, replace_same_string_multiple_times) {
  auto res = util::format("{0} {0} {1} {1}", "Hello", "World");
  ASSERT_EQ("Hello Hello World World", res);
}

TEST(format_test, replace_to_stringable_types) {
  auto res = util::format("{0},{1},{2},{3},{4},{5},{6},{7}", std::uint8_t{1},
                          std::uint16_t{2}, std::uint32_t{3}, std::uint64_t{4},
                          std::int8_t{5}, std::int16_t{6}, std::int32_t{7},
                          std::int64_t{8});
  ASSERT_EQ("1,2,3,4,5,6,7,8", res);
}

TEST(format_test, split_string_by_string_delimiter) {
  std::array<std::string, 9> expected{"1", "2", "3", "4", "5",
                                      "6", "7", "8", "9"};
  auto res = util::split("1--2--3--4--5--6--7--8--9", "--");
  ASSERT_EQ(res.size(), expected.size());
  ASSERT_TRUE(std::equal(res.begin(), res.end(), expected.begin()));
}

TEST(format_test, split_string_by_char_delimiter) {
  std::array<std::string, 9> expected{"1", "2", "3", "4", "5",
                                      "6", "7", "8", "9"};
  auto res = util::split("1-2-3-4-5-6-7-8-9", '-');
  ASSERT_EQ(res.size(), expected.size());
  ASSERT_TRUE(std::equal(res.begin(), res.end(), expected.begin()));
}
