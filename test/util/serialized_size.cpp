/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "util/serialized_size.hpp"

#include "net_test.hpp"

#include <algorithm>
#include <tuple>
#include <utility>

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

  auto visit(auto& f) {
    return f(s_, u8_, u16_, u32_, u64_, i8_, i16_, i32_, i64_, f_, d_);
  }
};

enum dummy_enum { one, two, three, four, five };

constexpr const std::uint8_t u8 = 0;
constexpr const std::uint16_t u16 = 0;
constexpr const std::uint32_t u32 = 0;
constexpr const std::uint64_t u64 = 0;
constexpr const std::int8_t i8 = 0;
constexpr const std::int16_t i16 = 0;
constexpr const std::int32_t i32 = 0;
constexpr const std::int64_t i64 = 0;

constexpr const float f = 0.0;
constexpr const double d = 0.0;

constexpr const std::size_t all_sizes_sum
  = (sizeof(std::uint8_t) + sizeof(std::uint16_t) + sizeof(std::uint32_t)
     + sizeof(std::uint64_t) + sizeof(std::int8_t) + sizeof(std::int16_t)
     + sizeof(std::int32_t) + sizeof(std::int64_t) + sizeof(float)
     + sizeof(double));

const std::string s1 = "hello";
const std::string s2 = "nhaqqhviueshvoiuenhvirejvre";

std::size_t string_size(const std::string& str) noexcept {
  return sizeof(std::size_t) + str.size();
}

} // namespace

TEST(serialized_size, empty_call) {
  serialized_size calculator;
  ASSERT_EQ(calculator(), 0);
}

TEST(serialized_size, integral_types) {
  ASSERT_EQ(serialized_size::calculate(u8), sizeof(std::uint8_t));
  ASSERT_EQ(serialized_size::calculate(u16), sizeof(std::uint16_t));
  ASSERT_EQ(serialized_size::calculate(u32), sizeof(std::uint32_t));
  ASSERT_EQ(serialized_size::calculate(u64), sizeof(std::uint64_t));
  ASSERT_EQ(serialized_size::calculate(i8), sizeof(std::int8_t));
  ASSERT_EQ(serialized_size::calculate(i16), sizeof(std::int16_t));
  ASSERT_EQ(serialized_size::calculate(i32), sizeof(std::int32_t));
  ASSERT_EQ(serialized_size::calculate(i64), sizeof(std::int64_t));
  ASSERT_EQ(serialized_size::calculate(f), sizeof(float));
  ASSERT_EQ(serialized_size::calculate(d), sizeof(double));
  ASSERT_EQ(serialized_size::calculate(u8, u16, u32, u64, i8, i16, i32, i64, f,
                                       d),
            all_sizes_sum);
}

TEST(serialized_size, byte) {
  ASSERT_EQ(serialized_size::calculate(std::byte{0}), sizeof(std::byte));
}

TEST(serialized_size, enum) {
  ASSERT_EQ(serialized_size::calculate(dummy_enum::one), sizeof(dummy_enum));
}

TEST(serialized_size, strings) {
  ASSERT_EQ(serialized_size::calculate(s1), string_size(s1));
  ASSERT_EQ(serialized_size::calculate(s2), string_size(s2));
  ASSERT_EQ(serialized_size::calculate(s1, s2, s2, s1),
            (2 * string_size(s1) + 2 * string_size(s2)));
}

TEST(serialized_size, mixed_types) {
  static const auto expected_size
    = (string_size(s1) + sizeof(std::uint8_t) + sizeof(std::uint16_t)
       + string_size(s2) + sizeof(std::int64_t) + sizeof(std::int16_t));
  ASSERT_EQ(serialized_size::calculate(s1, u8, u16, s2, i64, i16),
            expected_size);
}

TEST(serialized_size, pair) {
  static constexpr const std::size_t expected_size
    = (sizeof(std::uint32_t) + sizeof(std::uint64_t));
  ASSERT_EQ(serialized_size::calculate(
              std::make_pair(std::uint32_t{420}, std::uint64_t{69})),
            expected_size);
}

TEST(serialized_size, tuple) {
  static constexpr const std::size_t expected_size
    = (sizeof(std::uint32_t) + sizeof(std::uint64_t) + sizeof(std::uint8_t)
       + sizeof(std::uint16_t));
  ASSERT_EQ(serialized_size::calculate(
              std::make_tuple(std::uint32_t{420}, std::uint64_t{69},
                              std::uint8_t{42}, std::uint16_t{1337})),
            expected_size);
}

TEST(serialized_size, visit) {
  static const auto expected_size = serialized_size::calculate(s1, u8, u16, u32,
                                                               u64, i8, i16,
                                                               i32, i64, f, d);
  static const dummy_class dumm{s1, u8, u16, u32, u64, i8, i16, i32, i64, f, d};
  ASSERT_EQ(serialized_size::calculate(dumm), expected_size);
}

// -- Container tests ----------------------------------------------------------

TEST(serialized_size, c_style_array) {
  static constexpr const std::uint64_t u64_arr[10] = {0, 1, 2, 3, 4,
                                                      5, 6, 7, 8, 9};
  static constexpr const auto expected_size = sizeof(std::size_t)
                                              + sizeof(u64_arr);
  ASSERT_EQ(serialized_size::calculate(u64_arr), expected_size);
}

TEST(serialized_size, integral_container) {
  static constexpr const std::array<std::uint64_t, 10> u64_arr{0, 1, 2, 3, 4,
                                                               5, 6, 7, 8, 9};
  static constexpr const auto expected_size
    = sizeof(std::size_t) + (u64_arr.size() * sizeof(std::uint64_t));
  ASSERT_EQ(serialized_size::calculate(u64_arr), expected_size);
}

TEST(serialized_size, float_container) {
  static constexpr const std::array<float, 10> float_arr{0, 1, 2, 3, 4,
                                                         5, 6, 7, 8, 9};
  static constexpr const auto expected_size
    = sizeof(std::size_t) + (float_arr.size() * sizeof(float));
  ASSERT_EQ(serialized_size::calculate(float_arr), expected_size);
}

TEST(serialized_size, double_container) {
  static constexpr const std::array<double, 10> double_arr{0, 1, 2, 3, 4,
                                                           5, 6, 7, 8, 9};
  static constexpr const auto expected_size
    = sizeof(std::size_t) + (double_arr.size() * sizeof(double));
  ASSERT_EQ(serialized_size::calculate(double_arr), expected_size);
}

TEST(serialized_size, enum_container) {
  static constexpr const std::array<dummy_enum, 5> enum_arr{
    dummy_enum::one, dummy_enum::two, dummy_enum::three, dummy_enum::four,
    dummy_enum::five};
  static constexpr const auto expected_size
    = sizeof(std::size_t) + (enum_arr.size() * sizeof(dummy_enum));
  ASSERT_EQ(serialized_size::calculate(enum_arr), expected_size);
}

TEST(serialized_size, string_container) {
  static const std::array<std::string, 2> s_arr{s1, s2};
  static const auto expected_size = sizeof(std::size_t) + string_size(s1)
                                    + string_size(s2);
  ASSERT_EQ(serialized_size::calculate(s_arr), expected_size);
}

TEST(serialized_size, visitable_container) {
  static const std::array<dummy_class, 2> dummy_arr{
    dummy_class{s1, 1, 2, 3, 4, 5, 6, 7, 8, 9.0, 10.0},
    dummy_class{s2, 11, 12, 13, 14, 15, 16, 17, 18, 19.0, 20.0}};

  static const auto expected_size
    = sizeof(std::size_t)
      + (string_size(s1) + string_size(s2) + (2 * all_sizes_sum));
  ASSERT_EQ(serialized_size::calculate(dummy_arr), expected_size);
}
