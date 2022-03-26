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

TEST(serializer_test, serialize_int) {
  static constexpr const auto expected_result = make_byte_array(0x2A);

  byte_buffer buf;
  binary_serializer serializer{buf};
  serializer(std::uint8_t{42});
  ASSERT_EQ(buf.size(), expected_result.size());
  ASSERT_TRUE(std::equal(buf.begin(), buf.end(), expected_result.begin()));
}

TEST(serializer_test, serialize_string) {
  // static constexpr const auto expected_result
  //   = make_byte_array(0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 'H',
  //   'e',
  //                     'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd');

  // byte_buffer buf;
  // binary_serializer serializer{buf};
  // serializer("Hello World"s);
  // ASSERT_EQ(buf.size(), expected_result.size());
  // ASSERT_TRUE(std::equal(buf.begin(), buf.end(), expected_result.begin()));
}
