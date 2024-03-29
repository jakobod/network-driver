/**
 *  @author    Jakob Otto
 *  @file      format.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "util/format.hpp"

#include "net_test.hpp"

#include <algorithm>
#include <array>

TEST(format_test, replace_single_string) {
  const auto res = util::format("Hello {0}", "World");
  ASSERT_EQ("Hello World", res);
}

TEST(format_test, replace_multiple_strings) {
  const auto res = util::format("{0} {1}", "Hello", "World");
  ASSERT_EQ("Hello World", res);
}

TEST(format_test, replace_same_string_multiple_times) {
  const auto res = util::format("{0} {0} {1} {1}", "Hello", "World");
  ASSERT_EQ("Hello Hello World World", res);
}

TEST(format_test, replace_to_stringable_types) {
  const auto res = util::format("{0},{1},{2},{3},{4},{5},{6},{7}",
                                std::uint8_t{1}, std::uint16_t{2},
                                std::uint32_t{3}, std::uint64_t{4},
                                std::int8_t{5}, std::int16_t{6},
                                std::int32_t{7}, std::int64_t{8});
  ASSERT_EQ("1,2,3,4,5,6,7,8", res);
}

TEST(format_test, split_string_by_string_delimiter) {
  std::array<std::string, 9> expected{"1", "2", "3", "4", "5",
                                      "6", "7", "8", "9"};
  const auto res = util::split("1--2--3--4--5--6--7--8--9", "--");
  ASSERT_EQ(res.size(), expected.size());
  ASSERT_TRUE(std::equal(res.begin(), res.end(), expected.begin()));
}

TEST(format_test, split_string_by_char_delimiter) {
  std::array<std::string, 9> expected{"1", "2", "3", "4", "5",
                                      "6", "7", "8", "9"};
  const auto res = util::split("1-2-3-4-5-6-7-8-9", '-');
  ASSERT_EQ(res.size(), expected.size());
  ASSERT_TRUE(std::equal(res.begin(), res.end(), expected.begin()));
}

TEST(format_test, join_string) {
  const std::string expected{"1-2-3-4-5-6-7-8-9"};
  const auto res = util::join({"1", "2", "3", "4", "5", "6", "7", "8", "9"},
                              '-');
  ASSERT_EQ(expected, res);
}
