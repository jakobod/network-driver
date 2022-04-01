/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include <gtest/gtest.h>

#include "util/binary_deserializer.hpp"

#include "fwd.hpp"

#include <algorithm>

using namespace util;

using namespace std::string_literals;

namespace {

struct dummy_class {
  std::string s_;
  std::uint8_t u8_;
  std::uint16_t u16_;
  std::uint32_t u32_;
  std::uint64_t u64_;
  std::int8_t i8_;
  std::int16_t i16_;
  std::int32_t i32_;
  std::int64_t i64_;
  float f_;
  double d_;
};

bool operator==(const dummy_class& lhs, const dummy_class& rhs) {
  return (lhs.s_ == rhs.s_) && (lhs.u8_ == rhs.u8_) && (lhs.u16_ == rhs.u16_)
         && (lhs.u32_ == rhs.u32_) && (lhs.u64_ == rhs.u64_)
         && (lhs.i8_ == rhs.i8_) && (lhs.i16_ == rhs.i16_)
         && (lhs.i32_ == rhs.i32_) && (lhs.i64_ == rhs.i64_);
}

template <class Visitor>
auto visit(dummy_class& d, Visitor& f) {
  return f(d.s_, d.u8_, d.u16_, d.u32_, d.u64_, d.i8_, d.i16_, d.i32_, d.i64_,
           d.f_, d.d_);
}

} // namespace

TEST(binary_deserializer, empty_call) {
  byte_buffer buf;
  binary_deserializer deserializer{buf};
  ASSERT_NO_THROW(deserializer());
}

TEST(binary_deserializer, integer) {
  static constexpr const auto input
    = make_byte_array(0x01, 0x02, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x06, 0x00,
                      0x07, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00);

  binary_deserializer deserializer{input};
  std::uint8_t u8 = 0;
  std::uint16_t u16 = 0;
  std::uint32_t u32 = 0;
  std::uint64_t u64 = 0;
  std::int8_t i8 = 0;
  std::int16_t i16 = 0;
  std::int32_t i32 = 0;
  std::int64_t i64 = 0;

  ASSERT_NO_THROW(deserializer(u8, u16, u32, u64, i8, i16, i32, i64));

  ASSERT_EQ(u8, std::uint8_t{1});
  ASSERT_EQ(u16, std::uint16_t{2});
  ASSERT_EQ(u32, std::uint32_t{3});
  ASSERT_EQ(u64, std::uint64_t{4});
  ASSERT_EQ(i8, std::int8_t{5});
  ASSERT_EQ(i16, std::int16_t{6});
  ASSERT_EQ(i32, std::int32_t{7});
  ASSERT_EQ(i64, std::int64_t{8});
}

TEST(binary_deserializer, byte) {
  static constexpr const auto input = make_byte_array(0x2A, 0x45);
  std::byte b1;
  std::byte b2;
  binary_deserializer deserializer{input};
  ASSERT_NO_THROW(deserializer(b1, b2));
  ASSERT_EQ(b1, std::byte{42});
  ASSERT_EQ(b2, std::byte{69});
}

TEST(binary_deserializer, floats) {
  static constexpr const auto input = make_byte_array(0xd7, 0xa3, 0x70, 0x3d,
                                                      0xa, 0x4b, 0x7a, 0x40,
                                                      0xa, 0xd7, 0x8a, 0x42);

  double d;
  float f;
  binary_deserializer deserializer{input};
  ASSERT_NO_THROW(deserializer(d, f));
  ASSERT_EQ(d, double{420.69});
  ASSERT_EQ(f, float{69.42});
}

TEST(binary_deserializer, string) {
  static constexpr const auto input
    = make_byte_array(0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 'H', 'e',
                      'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', 0x05, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 'W', 'o', 'r', 'l',
                      'd', 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 'H',
                      'e', 'l', 'l', 'o');
  std::array<std::string, 3> result;
  binary_deserializer deserializer{input};
  ASSERT_NO_THROW(deserializer(result[0], result[1], result[2]));

  ASSERT_EQ(result[0], "Hello World"s);
  ASSERT_EQ(result[1], "World"s);
  ASSERT_EQ(result[2], "Hello"s);
}

TEST(binary_deserializer, pair) {
  static constexpr const auto input = make_byte_array(0xA4, 0x01, 0x00, 0x00,
                                                      0x45, 0x00, 0x00, 0x00,
                                                      0x00, 0x00, 0x00, 0x00);
  binary_deserializer deserializer{input};
  std::pair<std::uint32_t, std::uint64_t> p;
  ASSERT_NO_THROW(deserializer(p));
  ASSERT_EQ(p, std::make_pair(std::uint32_t{420}, std::uint64_t{69}));
}

TEST(binary_deserializer, tuple) {
  static constexpr const auto input
    = make_byte_array(0xA4, 0x01, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x2A, 0x39, 0x05);
  binary_deserializer deserializer{input};
  std::tuple<std::uint32_t, std::uint64_t, std::uint8_t, std::uint16_t> t;
  ASSERT_NO_THROW(deserializer(t));
  ASSERT_EQ(t, std::make_tuple(std::uint32_t{420}, std::uint64_t{69},
                               std::uint8_t{42}, std::uint16_t{1337}));
}

TEST(binary_deserializer, visit) {
  static constexpr const auto input = make_byte_array(
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 'H', 'e', 'l', 'l', 'o',
    0x01, 0x02, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05, 0x06, 0x00, 0x07, 0x00, 0x00, 0x00, 0x08, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x41, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x24, 0x40,

    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 'W', 'o', 'r', 'l', 'd',
    0x0B, 0x0C, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0F, 0x10, 0x00, 0x11, 0x00, 0x00, 0x00, 0x12, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x98, 0x41, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x34, 0x40);
  static const std::array<dummy_class, 2> expected_result{
    dummy_class{"Hello", 1, 2, 3, 4, 5, 6, 7, 8, 9.0, 10.0},
    dummy_class{"World", 11, 12, 13, 14, 15, 16, 17, 18, 19.0, 20.0}};
  binary_deserializer deserializer{input};
  dummy_class dummy;
  ASSERT_NO_THROW(deserializer(dummy));
  ASSERT_EQ(expected_result[0], dummy);
  ASSERT_NO_THROW(deserializer(dummy));
  ASSERT_EQ(expected_result[1], dummy);
}

TEST(binary_deserializer, vector) {
  static constexpr const auto input = make_byte_array(
    0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00);
  static constexpr const std::array<std::uint64_t, 10> expected_result{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  std::vector<std::uint64_t> buf;
  binary_deserializer deserializer{input};
  deserializer(buf);
  ASSERT_EQ(buf.size(), expected_result.size());
  ASSERT_TRUE(std::equal(buf.begin(), buf.end(), expected_result.begin()));
}
