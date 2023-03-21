/**
 *  @author    Jakob Otto
 *  @file      cli_parser.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "util/cli_parser.hpp"

#include "net_test.hpp"

using namespace util;
using namespace std::string_view_literals;

namespace {

constexpr const std::string_view id_opt1 = "opt1";
constexpr const std::string_view id_opt2 = "opt2";
constexpr const std::string_view id_opt3 = "opt3";

} // namespace

TEST(cli_parser, register_option) {
  {
    cli_parser parser;
    ASSERT_NO_THROW(parser.register_option(id_opt1.data(), "opt1", 'o', false));
    ASSERT_NO_THROW(parser.register_option(id_opt2.data(), "opt2", 'p', false));
    ASSERT_NO_THROW(parser.register_option(id_opt3.data(), "opt3", 'q', false));
  }
  {
    cli_parser parser;
    ASSERT_NO_THROW(parser.register_option(id_opt1.data(), "opt1", 'o', false)
                      .register_option(id_opt2.data(), "opt2", 'p', false)
                      .register_option(id_opt3.data(), "opt3", 'q', false));
  }
  {
    cli_parser parser;
    ASSERT_NO_THROW(parser.register_option(id_opt1.data(), "opt1", 'o', false));
    ASSERT_THROW(parser.register_option(id_opt1.data(), "opt2", 'p', false),
                 std::runtime_error);
  }
  {
    cli_parser parser;
    ASSERT_NO_THROW(parser.register_option(id_opt1.data(), "opt1", 'o', false));
    ASSERT_THROW(parser.register_option(id_opt2.data(), "opt1", 'p', false),
                 std::runtime_error);
  }
  {
    cli_parser parser;
    ASSERT_NO_THROW(parser.register_option(id_opt1.data(), "opt1", 'o', false));
    ASSERT_THROW(parser.register_option(id_opt2.data(), "opt2", 'o', false),
                 std::runtime_error);
  }
  {
    cli_parser parser;
    ASSERT_THROW(parser.register_option(id_opt1.data(), cli_parser::no_longform,
                                        cli_parser::no_shortform, false),
                 std::runtime_error);
  }
  {
    cli_parser parser;
    ASSERT_NO_THROW(
      parser.register_option(id_opt1.data(), "longform", 's', false));
    ASSERT_THROW(parser.register_option(id_opt1.data(), "longform", 's', false),
                 std::runtime_error);
  }
}

TEST(cli_parser, parse_longform_options) {
  {
    cli_parser parser;
    ASSERT_NO_THROW(parser.register_option(id_opt1.data(), "option",
                                           cli_parser::no_shortform, false));
    std::vector<const char*> args{"./program", "--option"};
    ASSERT_NO_THROW(parser.parse(args));
    EXPECT_EQ(parser.program_name(), "./program"sv);
    EXPECT_TRUE(parser.contains_option(id_opt1.data()));
    EXPECT_EQ(parser.num_option_values(id_opt1.data()), std::size_t{0});
  }
  {
    cli_parser parser;
    ASSERT_NO_THROW(parser.register_option(id_opt1.data(), "option",
                                           cli_parser::no_shortform, true));
    std::vector<const char*> args{"./program", "--option=argument"};
    ASSERT_NO_THROW(parser.parse(args));
    EXPECT_EQ(parser.program_name(), "./program"sv);
    EXPECT_TRUE(parser.contains_option(id_opt1.data()));
    EXPECT_EQ(parser.num_option_values(id_opt1.data()), std::size_t{1});
    EXPECT_EQ(parser.option_value<std::string>(id_opt1.data()), "argument");
  }

  {
    cli_parser parser;
    ASSERT_NO_THROW(parser.register_option(id_opt1.data(), "option",
                                           cli_parser::no_shortform, true));
    std::vector<const char*> args{"./program", "--option=argument1",
                                  "--option=argument2"};
    ASSERT_NO_THROW(parser.parse(args));
    EXPECT_EQ(parser.program_name(), "./program"sv);
    EXPECT_TRUE(parser.contains_option(id_opt1.data()));
    EXPECT_EQ(parser.num_option_values(id_opt1.data()), std::size_t{2});
    EXPECT_EQ(parser.option_value<std::string>(id_opt1.data(), 0), "argument1");
    EXPECT_EQ(parser.option_value<std::string>(id_opt1.data(), 1), "argument2");
  }
  {
    cli_parser parser;
    ASSERT_NO_THROW(parser.register_option(id_opt1.data(), "option",
                                           cli_parser::no_shortform, true));
    std::vector<const char*> args{"./program", "--option"};
    EXPECT_THROW(parser.parse(args), std::runtime_error);
  }
}

TEST(cli_parser, parse_shortform_options) {
  {
    cli_parser parser;
    ASSERT_NO_THROW(parser.register_option(
      id_opt1.data(), cli_parser::no_longform, 'o', false));
    std::vector<const char*> args{"./program", "-o"};
    ASSERT_NO_THROW(parser.parse(args));
    EXPECT_EQ(parser.program_name(), "./program"sv);
    EXPECT_TRUE(parser.contains_option(id_opt1.data()));
    EXPECT_EQ(parser.num_option_values(id_opt1.data()), std::size_t{0});
  }
  {
    cli_parser parser;
    ASSERT_NO_THROW(parser.register_option(id_opt1.data(),
                                           cli_parser::no_longform, 'o', true));
    std::vector<const char*> args{"./program", "-oargument"};
    ASSERT_NO_THROW(parser.parse(args));
    EXPECT_EQ(parser.program_name(), "./program"sv);
    EXPECT_TRUE(parser.contains_option(id_opt1.data()));
    EXPECT_EQ(parser.num_option_values(id_opt1.data()), std::size_t{1});
    EXPECT_EQ(parser.option_value<std::string>(id_opt1.data()), "argument");
  }
  {
    cli_parser parser;
    ASSERT_NO_THROW(parser.register_option(id_opt1.data(),
                                           cli_parser::no_longform, 'o', true));
    std::vector<const char*> args{"./program", "-o", "argument"};
    ASSERT_NO_THROW(parser.parse(args));
    EXPECT_EQ(parser.program_name(), "./program"sv);
    EXPECT_TRUE(parser.contains_option(id_opt1.data()));
    EXPECT_EQ(parser.num_option_values(id_opt1.data()), std::size_t{1});
    EXPECT_EQ(parser.option_value<std::string>(id_opt1.data()), "argument");
  }
  {
    cli_parser parser;
    ASSERT_NO_THROW(
      parser
        .register_option(id_opt1.data(), cli_parser::no_longform, 'a', false)
        .register_option(id_opt2.data(), cli_parser::no_longform, 'b', false)
        .register_option(id_opt3.data(), cli_parser::no_longform, 'c', false));
    std::vector<const char*> args{"./program", "-abc"};
    ASSERT_NO_THROW(parser.parse(args));
    EXPECT_EQ(parser.program_name(), "./program"sv);
    EXPECT_TRUE(parser.contains_option(id_opt1.data()));
    EXPECT_TRUE(parser.contains_option(id_opt2.data()));
    EXPECT_TRUE(parser.contains_option(id_opt3.data()));
  }
}

TEST(cli_parser, option_value_conversion) {
  // Converting argument to integral type
  {
    cli_parser parser;
    ASSERT_NO_THROW(
      parser.register_option(id_opt1.data(), "option", 'o', true));
    std::vector<const char*> args{"./program", "--option=42"};
    ASSERT_NO_THROW(parser.parse(args));
    EXPECT_EQ(parser.program_name(), "./program"sv);
    EXPECT_TRUE(parser.contains_option(id_opt1.data()));
    EXPECT_EQ(parser.num_option_values(id_opt1.data()), std::size_t{1});
    EXPECT_EQ(parser.option_value<std::string>(id_opt1.data()), "42");
    EXPECT_EQ(parser.option_value<int>(id_opt1.data()), int{42});
    EXPECT_EQ(parser.option_value<std::size_t>(id_opt1.data()),
              std::size_t{42});
  }
  // Converting argument to floating type
  {
    cli_parser parser;
    ASSERT_NO_THROW(
      parser.register_option(id_opt1.data(), "option", 'o', true));
    std::vector<const char*> args{"./program", "--option=42.69"};
    ASSERT_NO_THROW(parser.parse(args));
    EXPECT_EQ(parser.program_name(), "./program"sv);
    EXPECT_TRUE(parser.contains_option(id_opt1.data()));
    EXPECT_EQ(parser.num_option_values(id_opt1.data()), std::size_t{1});
    EXPECT_EQ(parser.option_value<std::string>(id_opt1.data()), "42.69");
    EXPECT_EQ(parser.option_value<float>(id_opt1.data()), float{42.69});
    EXPECT_EQ(parser.option_value<double>(id_opt1.data()), double{42.69});
  }
  // Converting to unsupported type
  {
    cli_parser parser;
    ASSERT_NO_THROW(
      parser.register_option(id_opt1.data(), "option", 'o', true));
    std::vector<const char*> args{"./program", "--option=42.69"};
    ASSERT_NO_THROW(parser.parse(args));
    EXPECT_EQ(parser.program_name(), "./program"sv);
    EXPECT_TRUE(parser.contains_option(id_opt1.data()));
    EXPECT_EQ(parser.num_option_values(id_opt1.data()), std::size_t{1});
    EXPECT_EQ(parser.option_value<std::string>(id_opt1.data()), "42.69");
    struct dummy {};
    EXPECT_THROW(parser.option_value<dummy>(id_opt1.data()),
                 std::runtime_error);
  }
  // Converting to unsupported type
  {
    cli_parser parser;
    ASSERT_NO_THROW(
      parser.register_option(id_opt1.data(), "option", 'o', true));
    std::vector<const char*> args{"./program", "--option=hello", "-o", "other"};
    ASSERT_NO_THROW(parser.parse(args));
    EXPECT_EQ(parser.program_name(), "./program"sv);
    EXPECT_TRUE(parser.contains_option(std::string(id_opt1)));
    EXPECT_EQ(parser.num_option_values(id_opt1.data()), std::size_t{2});
    auto conv = [](const std::string& str) {
      return (str == "hello") ? "world" : "what";
    };
    EXPECT_EQ(parser.option_value(id_opt1.data(), conv, 0), "world");
    EXPECT_EQ(parser.option_value(id_opt1.data(), conv, 1), "what");
  }
}
