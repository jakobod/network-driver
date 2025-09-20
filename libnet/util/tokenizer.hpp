/**
 *  @author    Jakob Otto
 *  @file      tokenizer.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <algorithm>
#include <compare>
#include <fstream>
#include <functional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace util {

template <class Policy>
concept tokenizer_policy = requires {
  typename Policy::token_type;
  typename Policy::predicate;
  typename Policy::predicate_entry;
} && requires {
  { Policy::predicates() } -> std::ranges::range;
} && requires {
  requires std::same_as<
    std::remove_cvref_t<decltype(Policy::fallback())>,
    std::pair<typename Policy::token_type, typename Policy::predicate>>;
};

template <tokenizer_policy Policy>
class tokenizer {
public:
  /// @brief A token parsed from the input data
  struct token {
    typename Policy::token_type type{};
    std::string value{};
    std::size_t line_number{};
    std::size_t token_number{};

    std::strong_ordering operator<=>(const token& other) const noexcept
      = default;

    friend std::ostream& operator<<(std::ostream& os, const token& t)
      requires requires { to_string(t.type); }
    {
      return os << "{" << to_string(t.type) << " : \"" << t.value
                << "\"} at line=" << t.line_number
                << " token_number=" << t.token_number;
    }
  };

  using token_list = std::vector<token>;

  tokenizer() = default;

  bool tokenize_file(std::string_view file_path) {
    std::ifstream is{file_path.data()};
    if (!is) {
      return false;
    }
    std::string line;
    while (std::getline(is, line)) {
      tokenize_line(line);
    }
    return true;
  }

  void tokenize_line(std::string_view input) {
    std::size_t num_tokens_found = 0;
    while (!input.empty()) {
      // skip whitespace
      const auto first_non_space = input.find_first_not_of(" ");
      if (first_non_space == std::string_view::npos) {
        break;
      }
      input = input.substr(first_non_space);

      auto matched_something = false;
      for (const auto& [token_type, predicate] : Policy::predicates()) {
        if (const auto length = predicate(input); length != 0) {
          // found something!
          tokens_.emplace_back(token_type, std::string{input.substr(0, length)},
                               current_line_number, num_tokens_found++);
          input = input.substr(length);
          matched_something = true;
          break;
        }
      }
      if (!matched_something) {
        const auto& [token_type, predicate] = Policy::fallback();
        if (const auto length = predicate(input); length != 0) {
          // found something!
          tokens_.emplace_back(token_type, std::string{input.substr(0, length)},
                               current_line_number, num_tokens_found++);
          input = input.substr(length);
        } else {
          input = input.substr(1);
        }
      }
    }

    ++current_line_number;
  }

  const token_list& tokens() const noexcept { return tokens_; }

private:
  token_list tokens_{};
  std::size_t current_line_number{0};
};

namespace matchers {

static constexpr std::size_t
match_until_whitespace(std::string_view str) noexcept {
  const auto first_space = str.find_first_of(" ");
  return (first_space == std::string_view::npos) ? str.size() : first_space;
}

template <char Character>
static constexpr std::size_t match_single_char(std::string_view str) noexcept {
  return str.starts_with(Character) ? 1 : 0;
}

template <const char* String>
static constexpr std::size_t match_string(std::string_view str) noexcept {
  static constexpr std::string_view string_to_match{String};
  return str.starts_with(string_to_match) ? string_to_match.size() : 0;
}

static constexpr std::size_t match_integer(std::string_view str) noexcept {
  const auto first_non_digit = str.find_first_not_of("0123456789");
  if ((first_non_digit != std::string_view::npos)
      && (str.at(first_non_digit) != ' ')) {
    return 0;
  }
  return (first_non_digit == std::string_view::npos) ? str.size()
                                                     : first_non_digit;
}

static constexpr std::size_t match_bool(std::string_view str) noexcept {
  if (str.starts_with("true")) {
    return 4;
  } else if (str.starts_with("false")) {
    return 5;
  } else {
    return 0;
  }
}

static constexpr std::size_t match_float(std::string_view str) noexcept {
  const auto first_non_digit = str.find_first_not_of("0123456789");
  if ((first_non_digit == std::string_view::npos)
      || (str.at(first_non_digit) != '.')) {
    return 0;
  }
  auto substr = str.substr(first_non_digit + 1);
  const auto second_integer_length = match_integer(substr);
  return (second_integer_length == 0)
           ? 0
           : first_non_digit + second_integer_length + 1;
}

} // namespace matchers

} // namespace util
