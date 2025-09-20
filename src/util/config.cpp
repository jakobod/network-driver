/**
 *  @author    Jakob Otto
 *  @file      config.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "util/config.hpp"

#include "util/assert.hpp"
#include "util/error.hpp"
#include "util/error_or.hpp"
#include "util/tokenizer.hpp"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <ranges>
#include <stack>
#include <string_view>

using namespace std::string_literals;

using dictionary = util::config::dictionary;

namespace {

enum class token {
  unknown,
  integer,
  floating_point_number,
  string,
  equals,
  opening_brace,
  closing_brace,
  boolean,
  namespace_keyword,
};

constexpr std::string_view to_string(token type) noexcept {
  switch (type) {
    case token::unknown:
      return "unknown";
    case token::integer:
      return "integer";
    case token::floating_point_number:
      return "floating_point_number";
    case token::string:
      return "string";
    case token::equals:
      return "equals";
    case token::opening_brace:
      return "opening_brace";
    case token::closing_brace:
      return "closing_brace";
    case token::boolean:
      return "boolean";
    case token::namespace_keyword:
      return "namespace";
    default:
      DEBUG_ONLY_ASSERT(false, "Unhandled token");
      return "????";
  }
}

constexpr const char namespace_identifier[] = "namespace";

struct config_tokenizer_policy {
  using token_type      = token;
  using predicate       = std::function<std::size_t(std::string_view)>;
  using predicate_entry = std::pair<token_type, predicate>;

  static const std::array<predicate_entry, 7>& predicates() noexcept {
    using namespace util::matchers;
    static const std::array<predicate_entry, 7> predicates{
      predicate_entry{token_type::opening_brace, match_single_char<'{'>},
      predicate_entry{token_type::closing_brace, match_single_char<'}'>},
      predicate_entry{token_type::equals, match_single_char<'='>},
      predicate_entry{token_type::integer, match_integer},
      predicate_entry{token_type::floating_point_number, match_float},
      predicate_entry{token_type::boolean, match_bool},
      predicate_entry{token_type::namespace_keyword,
                      match_string<namespace_identifier>},
    };
    return predicates;
  }

  static const predicate_entry& fallback() noexcept {
    static const predicate_entry fallback{
      token_type::string, util::matchers::match_until_whitespace};
    return fallback;
  }
};

using tokenizer_type = util::tokenizer<config_tokenizer_policy>;
using token_list     = tokenizer_type::token_list;

util::error make_parser_error(std::string msg, std::size_t line,
                              std::size_t token_number) {
  msg += (" at line "s + std::to_string(line) + ":"s
          + std::to_string(token_number));
  return util::error{util::error_code::parser_error, std::move(msg)};
}

util::error parse_value(dictionary& dict, token type, std::string key,
                        std::string_view str) {
  switch (type) {
    case token::integer: {
      std::int64_t result{};
      auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(),
                                       result);
      if (ec != std::errc{}) {
        return util::error{util::error_code::parser_error,
                           "Failed to parse integer"};
      }
      dict.emplace(key, result);
      break;
    }

    case token::floating_point_number:
      dict.emplace(key, std::stod(std::string{str}));
      break;

    case token::string:
      dict.emplace(key, std::string{str});
      break;

    case token::boolean:
      dict.emplace(key, (str == "true"));
      break;

    default:
      return util::error{util::error_code::parser_error};
  }
  return util::none;
}

util::error_or<dictionary> parse_file(std::string_view config_path) {
  tokenizer_type tokenizer;
  tokenizer.tokenize_file(config_path.data());

  // Parsing state
  dictionary dict;
  std::string current_namespace;
  std::stack<std::size_t> namespace_lengths;

  const auto& tokens = tokenizer.tokens();
  for (auto it = tokens.begin(); it != tokens.end(); ++it) {
    const auto& [token_type, token_value, line_number, token_number] = *it;

    switch (token_type) {
      case token::unknown:
        return make_parser_error("unknown token \""s + token_value + "\" found",
                                 line_number, token_number);
        break;
      case token::integer:
        return make_parser_error("Misplaced integer value found", line_number,
                                 token_number);
        break;
      case token::floating_point_number:
        return make_parser_error("Misplaced floating point value found",
                                 line_number, token_number);
        break;
      case token::equals:
        return make_parser_error("Misplaced assignment operator found",
                                 line_number, token_number);
        break;
      case token::opening_brace:
        // Append a zero length so nothing is removed on closing brace.
        namespace_lengths.emplace(0);
        break;
      case token::closing_brace:
        current_namespace.resize(current_namespace.size()
                                 - (namespace_lengths.top() + 1));
        namespace_lengths.pop();
        break;
      case token::boolean:
        return make_parser_error("Misplaced boolean value found", line_number,
                                 token_number);
        break;

      case token::namespace_keyword: {
        // must be followed by a string and an opening brace!
        const auto& namespace_name = *(++it);
        const auto& opening_brace  = *(++it);
        if (namespace_name.type != token::string) {
          return make_parser_error("missing name for namespace declaration",
                                   line_number, token_number);
        }
        if (opening_brace.type != token::opening_brace) {
          return make_parser_error(
            "missing opening brace after namespace declaration", line_number,
            token_number);
        }
        current_namespace += namespace_name.value + ".";
        namespace_lengths.emplace(namespace_name.value.size());
        break;
      }

      case token::string: {
        // must be followed by a string and an opening brace!
        const auto& assignment_operator = *(++it);
        const auto& value               = *(++it);
        if (assignment_operator.type != token::equals) {
          return make_parser_error(
            "missing assignment operator for variable assignment", line_number,
            token_number);
        }
        if ((value.type != token::integer)
            && (value.type != token::floating_point_number)
            && (value.type != token::string)
            && (value.type != token::boolean)) {
          return make_parser_error(
            "Can only assign boolean, integers, strings, or floats",
            line_number, token_number);
        }

        auto key = current_namespace + token_value;
        if (auto err = parse_value(dict, value.type, std::move(key),
                                   value.value)) {
          return err;
        }
        break;
      }
      default:
        DEBUG_ONLY_ASSERT(false, "Unhandled token");
        break;
    }
  }
  if (!namespace_lengths.empty()) {
    return util::error{util::error_code::parser_error,
                       "Missing closing brace for namespace"};
  }
  return dict;
}

} // namespace

namespace util {

util::error config::parse(std::string_view config_path) {
  auto res = parse_file(config_path);
  if (auto* err = util::get_error(res)) {
    return *err;
  }
  config_values_ = std::move(std::get<dictionary>(res));
  return util::none;
}

} // namespace util
