/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include <gtest/gtest.h>

#include "util/binary_serializer.hpp"

#include "fwd.hpp"

#include <algorithm>

using namespace util;

using namespace std::string_literals;

namespace {

class dummy {
  std::string s_;
  std::uint64_t u64_;
};

} // namespace

// TEST(serializer_test, int) {
//   static constexpr const auto expected_result = make_byte_array(0x2A);

//   byte_buffer buf;
//   binary_serializer serializer{buf};
//   serializer(std::uint8_t{42});
//   ASSERT_EQ(buf.size(), expected_result.size());
//   ASSERT_TRUE(std::equal(buf.begin(), buf.end(), expected_result.begin()));
// }

// TEST(serializer_test, string) {
//   // clang-format off
//   const std::string input{"Hello World"};
//   static constexpr const auto expected_result
//     = make_byte_array(0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//                       'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd');
//   // clang-format on

//   byte_buffer buf;
//   binary_serializer serializer{buf};
//   serializer(input);
//   ASSERT_EQ(buf.size(), expected_result.size());
//   ASSERT_TRUE(std::equal(buf.begin(), buf.end(), expected_result.begin()));
// }

// TEST(serializer_test, vector) {
//   static constexpr const std::array<std::uint64_t, 10> input{0, 1, 2, 3, 4,
//                                                              5, 6, 7, 8, 9};
//   static constexpr const auto expected_result = make_byte_array(
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

//   byte_buffer buf;
//   binary_serializer serializer{buf};
//   serializer(input);
//   ASSERT_EQ(buf.size(), expected_result.size());
//   ASSERT_TRUE(std::equal(buf.begin(), buf.end(), expected_result.begin()));
// }
