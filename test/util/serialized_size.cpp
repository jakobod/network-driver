/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include <gtest/gtest.h>

#include "util/serialized_size.hpp"

#include "fwd.hpp"

#include <algorithm>

using namespace util;

using namespace std::string_literals;

namespace {

struct dummy {
  std::string s_;
  std::uint8_t u8_;
  std::uint16_t u16_;
  std::uint32_t u32_;
  std::uint64_t u64_;
  std::int8_t i8_;
  std::int16_t i16_;
  std::int32_t i32_;
  std::int64_t i64_;
};

template <class Visitor>
auto visit(const dummy& d, Visitor& f) {
  return f(d.s_, d.u8_, d.u16_, d.u32_, d.u64_, d.i8_, d.i16_, d.i32_, d.i64_);
}

constexpr const std::uint8_t u8 = 0;
constexpr const std::uint16_t u16 = 0;
constexpr const std::uint32_t u32 = 0;
constexpr const std::uint64_t u64 = 0;
constexpr const std::int8_t i8 = 0;
constexpr const std::int16_t i16 = 0;
constexpr const std::int32_t i32 = 0;
constexpr const std::int64_t i64 = 0;

const std::string s1 = "hello";
const std::string s2 = "nhaqqhviueshvoiuenhvirejvre";

// containers

constexpr const std::array<std::uint64_t, 10> u64_arr{0, 1, 2, 3, 4,
                                                      5, 6, 7, 8, 9};

const std::array<std::string, 2> s_arr{s1, s2};

std::size_t string_size(const std::string& str) noexcept {
  return sizeof(std::size_t) + str.size();
}

} // namespace

TEST(serialized_size, integers) {
  ASSERT_EQ(serialized_size::calculate(u8), sizeof(std::uint8_t));
  ASSERT_EQ(serialized_size::calculate(u16), sizeof(std::uint16_t));
  ASSERT_EQ(serialized_size::calculate(u32), sizeof(std::uint32_t));
  ASSERT_EQ(serialized_size::calculate(u64), sizeof(std::uint64_t));
  ASSERT_EQ(serialized_size::calculate(i8), sizeof(std::int8_t));
  ASSERT_EQ(serialized_size::calculate(i16), sizeof(std::int16_t));
  ASSERT_EQ(serialized_size::calculate(i32), sizeof(std::int32_t));
  ASSERT_EQ(serialized_size::calculate(i64), sizeof(std::int64_t));
  ASSERT_EQ(serialized_size::calculate(u8, u16, u32, u64, i8, i16, i32, i64),
            (sizeof(std::uint8_t) + sizeof(std::uint16_t)
             + sizeof(std::uint32_t) + sizeof(std::uint64_t)
             + sizeof(std::int8_t) + sizeof(std::int16_t) + sizeof(std::int32_t)
             + sizeof(std::int64_t)));
}

TEST(serialized_size, strings) {
  ASSERT_EQ(serialized_size::calculate(s1), string_size(s1));
  ASSERT_EQ(serialized_size::calculate(s2), string_size(s2));
  ASSERT_EQ(serialized_size::calculate(s1, s2),
            (string_size(s1) + string_size(s2)));
}

TEST(serialized_size, mixed_types) {
  static const auto expected_size
    = (string_size(s1) + sizeof(std::uint8_t) + sizeof(std::uint16_t)
       + string_size(s2) + sizeof(std::int64_t) + sizeof(std::int16_t));
  ASSERT_EQ(serialized_size::calculate(s1, u8, u16, s2, i64, i16),
            expected_size);
}

TEST(serialized_size, visit) {
  static const auto expected_size
    = serialized_size::calculate(s1, u8, u16, u32, u64, i8, i16, i32, i64);
  static const dummy d{s1, u8, u16, u32, u64, i8, i16, i32, i64};
  ASSERT_EQ(serialized_size::calculate(d), expected_size);
}

// TEST(serialized_size, container) {
//   static constexpr const auto expected_size
//     = (u64_arr.size() * sizeof(std::uint64_t));
//   ASSERT_EQ(serialized_size::calculate(u64_arr), expected_size);
// }
