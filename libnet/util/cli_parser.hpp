/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <functional>
#include <map>
#include <ostream>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace util {

class cli_parser {
public:
  struct option {
    option(std::string id, std::string longform, char shortform,
           bool has_value);

    const std::string id_;
    const std::string longform_;
    const char shortform_;
    const bool has_value_;

    bool operator==(const option& other) const noexcept;
    bool operator==(const std::string_view& longform) const noexcept;
    bool operator==(char shortform) const noexcept;
    bool operator<(const option& other) const noexcept;
    bool operator>(const option& other) const noexcept;
  };

  using arg_span = std::span<const char*>;

private:
  using option_set = std::set<option, std::not_equal_to<>>;
  using string_list = std::vector<std::string>;
  using result_map = std::map<std::string, string_list>;
  using arg_iterator = arg_span::iterator;

public:
  static constexpr const char* no_longform = "";
  static constexpr const char no_shortform = '\0';

  // -- Adding and parsing options ---------------------------------------------

  cli_parser& add_option(option opt);

  template <class... Ts>
  cli_parser& add_option(Ts&&... xs) {
    return add_option(option{std::forward<Ts>(xs)...});
  }

  void parse(arg_span args);

  void parse(int argc, const char** argv);

  // -- Queries ----------------------------------------------------------------

  const std::string& program_name() const noexcept;

  bool contains_option(const std::string& id) const;

  std::size_t num_option_values(const std::string& id) const;

  bool has_option_value(const std::string& id) const;

  string_list& option_values(const std::string& id);

  const string_list& option_values(const std::string& id) const;

  template <class T>
  T option_value(const std::string&, std::size_t = 0) const {
    throw std::runtime_error{"Casting to type T is not implemented"};
    return {};
  }

  template <>
  std::string option_value(const std::string& id, std::size_t pos) const {
    return parsed_.at(id).at(pos);
  }

  template <>
  int option_value(const std::string& id, std::size_t pos) const {
    return std::stoi(parsed_.at(id).at(pos));
  }

  template <>
  std::size_t option_value(const std::string& id, std::size_t pos) const {
    return std::stol(parsed_.at(id).at(pos));
  }

  template <>
  float option_value(const std::string& id, std::size_t pos) const {
    return std::stof(parsed_.at(id).at(pos));
  }

  template <>
  double option_value(const std::string& id, std::size_t pos) const {
    return std::stod(parsed_.at(id).at(pos));
  }

  template <class ConversionFunc>
  auto option_value(const std::string& id, ConversionFunc conv,
                    std::size_t pos = 0) const {
    return conv(parsed_.at(id).at(pos));
  }

  const option_set& options() const { return options_; }

private:
  template <class Option>
  arg_span parse_opt(Option opt, arg_span args) {
    using namespace std::string_literals;
    const auto pos = std::find(options_.begin(), options_.end(), opt);
    if (pos == options_.end())
      return args;
    parsed_.emplace(pos->id_, string_list{});

    if (pos->has_value_) {
      args = args.subspan(1);
      if (args.empty() || std::string_view{args.front()}.starts_with("-"))
        throw std::runtime_error("Missing an expected argument for option \""s
                                 + pos->id_);
      parsed_.at(pos->id_).emplace_back(args.front());
    }

    return args;
  }

  option_set options_;
  std::string program_name_;
  result_map parsed_;
};

} // namespace util
