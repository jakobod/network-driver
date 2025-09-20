/**
 *  @author    Jakob Otto
 *  @file      config.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "util/tokenizer.hpp"

#include "net_test.hpp"

#include <fstream>

using namespace std::string_view_literals;

using util::tokenizer;
using namespace util::matchers;

namespace {

struct temp_file {
  temp_file(std::string path, std::string_view content)
    : path_{std::move(path)} {
    std::ofstream os{path_};
    os << content;
  }

  ~temp_file() { std::remove(path_.c_str()); }

private:
  const std::string path_;
};

enum class test_token : std::uint8_t {
  opening_bracket,
  closing_bracket,
  integer,
  floating_point_number,
  equals,
  string,
};

std::string_view to_string(test_token token) {
  switch (token) {
    case test_token::opening_bracket:
      return "opening_bracket";
    case test_token::closing_bracket:
      return "closing_bracket";
    case test_token::integer:
      return "integer";
    case test_token::floating_point_number:
      return "floating_point_number";
    case test_token::equals:
      return "equals";
    case test_token::string:
      return "string";
    default:
      return "????";
  }
}

struct test_policy {
  using token_type = test_token;
  using predicate = std::function<std::size_t(std::string_view)>;
  using predicate_entry = std::pair<token_type, predicate>;

  static const std::array<predicate_entry, 5>& predicates() noexcept {
    static const std::array<predicate_entry, 5> predicates{
      predicate_entry{test_token::opening_bracket, match_single_char<'{'>},
      predicate_entry{test_token::closing_bracket, match_single_char<'}'>},
      predicate_entry{test_token::equals, match_single_char<'='>},
      predicate_entry{test_token::integer, match_integer},
      predicate_entry{test_token::floating_point_number, match_float},
    };
    return predicates;
  }

  static const predicate_entry& fallback() noexcept {
    static const predicate_entry fallback{test_token::string,
                                          match_until_whitespace};
    return fallback;
  }
};

using tokenizer_type = tokenizer<test_policy>;

constexpr const char hello[] = "hello";

} // namespace

TEST(tokenizer, match_until_whitespace) {
  EXPECT_EQ(match_until_whitespace("foo"sv), 3);
  EXPECT_EQ(match_until_whitespace("f"sv), 1);
  EXPECT_EQ(match_until_whitespace("         "sv), 0);
  EXPECT_EQ(match_until_whitespace("     frwfcrw    "sv), 0);

  EXPECT_EQ(match_until_whitespace("Soemt qwhebfewinf wiuncwec"sv), 5);
  EXPECT_EQ(match_until_whitespace("ffffffm          "sv), 7);
}

TEST(tokenizer, match_single_char) {
  EXPECT_EQ(match_single_char<'f'>("foo"sv), 1);
  EXPECT_EQ(match_single_char<'f'>("f"sv), 1);
  EXPECT_EQ(match_single_char<'f'>(""sv), 0);
  EXPECT_EQ(match_single_char<'f'>("         "sv), 0);
  EXPECT_EQ(match_single_char<'f'>("bfff"sv), 0);
  EXPECT_EQ(match_single_char<'f'>("ffffff"sv), 1);
}

TEST(tokenizer, match_string) {
  EXPECT_EQ(match_string<hello>("hello"sv), 5);
  EXPECT_EQ(match_string<hello>("hello world"sv), 5);
  EXPECT_EQ(match_string<hello>("hello world this is something more"sv), 5);
  EXPECT_EQ(match_string<hello>(" hello world"sv), 0);
  EXPECT_EQ(match_string<hello>("something hello"sv), 0);
}

TEST(tokenizer, match_integer) {
  EXPECT_EQ(match_integer("foo"sv), 0);
  EXPECT_EQ(match_integer("f"sv), 0);
  EXPECT_EQ(match_integer("         "sv), 0);
  EXPECT_EQ(match_integer("     frwfcrw    "sv), 0);
  EXPECT_EQ(match_integer("12355"sv), 5);
  EXPECT_EQ(match_integer("     12345    "sv), 0);
  EXPECT_EQ(match_integer("     12345"sv), 0);
  EXPECT_EQ(match_integer("12345frwfcrw    "sv), 0);
  EXPECT_EQ(match_integer("12345    "sv), 5);
}

TEST(tokenizer, match_float) {
  EXPECT_EQ(match_float("foo"sv), 0);
  EXPECT_EQ(match_float("f"sv), 0);
  EXPECT_EQ(match_float("         "sv), 0);
  EXPECT_EQ(match_float("     frwfcrw    "sv), 0);
  EXPECT_EQ(match_float("12355.12345"sv), 11);
  EXPECT_EQ(match_float("     12345.12345    "sv), 0);
  EXPECT_EQ(match_float("     12345.12345"sv), 0);
  EXPECT_EQ(match_float("12345.12345frwfcrw    "sv), 0);
  EXPECT_EQ(match_float("12345.12345    "sv), 11);
}

TEST(tokenizer, tokenize_file) {
  tokenizer_type tokenizer;

  const temp_file config_file{"/tmp/config.cfg", R"(namespace {
      some numbers are just weird
      12346571698 123.557
      123.
      hello = world
    }
  )"};

  EXPECT_TRUE(tokenizer.tokenize_file("/tmp/config.cfg"));

  static const tokenizer_type::token_list expected_result{
    {test_token::string, "namespace", 0, 0},
    {test_token::opening_bracket, "{", 0, 1},
    {test_token::string, "some", 1, 0},
    {test_token::string, "numbers", 1, 1},
    {test_token::string, "are", 1, 2},
    {test_token::string, "just", 1, 3},
    {test_token::string, "weird", 1, 4},
    {test_token::integer, "12346571698", 2, 0},
    {test_token::floating_point_number, "123.557", 2, 1},
    {test_token::string, "123.", 3, 0},
    {test_token::string, "hello", 4, 0},
    {test_token::equals, "=", 4, 1},
    {test_token::string, "world", 4, 2},
    {test_token::closing_bracket, "}", 5, 0},
  };

  EXPECT_EQ(tokenizer.tokens(), expected_result);
}

TEST(tokenizer, parse_tokens) {
  tokenizer_type tokenizer;

  ASSERT_NO_THROW(tokenizer.tokenize_line("namespace {"));
  ASSERT_NO_THROW(tokenizer.tokenize_line("  some numbers are just weird"));
  ASSERT_NO_THROW(tokenizer.tokenize_line("  12346571698 123.557"));
  ASSERT_NO_THROW(tokenizer.tokenize_line("  123."));
  ASSERT_NO_THROW(tokenizer.tokenize_line("  hello = world"));
  ASSERT_NO_THROW(tokenizer.tokenize_line("}"));

  static const tokenizer_type::token_list expected_result{
    {test_token::string, "namespace", 0, 0},
    {test_token::opening_bracket, "{", 0, 1},
    {test_token::string, "some", 1, 0},
    {test_token::string, "numbers", 1, 1},
    {test_token::string, "are", 1, 2},
    {test_token::string, "just", 1, 3},
    {test_token::string, "weird", 1, 4},
    {test_token::integer, "12346571698", 2, 0},
    {test_token::floating_point_number, "123.557", 2, 1},
    {test_token::string, "123.", 3, 0},
    {test_token::string, "hello", 4, 0},
    {test_token::equals, "=", 4, 1},
    {test_token::string, "world", 4, 2},
    {test_token::closing_bracket, "}", 5, 0},
  };

  EXPECT_EQ(tokenizer.tokens(), expected_result);
}
