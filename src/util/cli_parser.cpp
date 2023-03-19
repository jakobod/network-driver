/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "util/cli_parser.hpp"

#include "util/format.hpp"

#include <compare>
#include <iostream>
#include <span>

using namespace std::string_literals;

using option = util::cli_parser::option;

namespace util {

option::option(std::string id, std::string longform, char shortform,
               bool has_value)
  : id_{std::move(id)},
    longform_{std::move(longform)},
    shortform_{shortform},
    has_value_{has_value} {
  if ((longform_ == no_longform) && (shortform_ == no_shortform))
    throw std::runtime_error("No long or shortform option set for option \""
                             + id_ + "\"");
}

bool option::operator==(const option& other) const noexcept {
  return (id_ == other.id_)
         || ((longform_ != cli_parser::no_longform)
             && (longform_ == other.longform_))
         || ((shortform_ != cli_parser::no_shortform)
             && (shortform_ == other.shortform_));
}

bool option::operator==(const std::string_view& longform) const noexcept {
  return longform_ == longform;
}

bool option::operator==(char shortform) const noexcept {
  return shortform_ == shortform;
}

bool option::operator<(const option& other) const noexcept {
  return (id_ < other.id_) || (longform_ < other.longform_)
         || (shortform_ < other.shortform_);
}

bool option::operator>(const option& other) const noexcept {
  return (id_ > other.id_) || (longform_ > other.longform_)
         || (shortform_ > other.shortform_);
}

cli_parser& cli_parser::register_option(option opt) {
  auto [it, success] = options_.emplace(std::move(opt));
  if (!success)
    throw std::runtime_error("Option already contained");
  return *this;
}

void cli_parser::parse(arg_span args) {
  program_name_ = std::string{args.front()};
  // Skip the program name

  for (args = args.subspan(1); !args.empty(); args = args.subspan(1)) {
    const std::string_view opt{args.front()};
    // Check wether this is a short/longform option and strip the dashes
    if (opt.starts_with("--"))
      parse_longform_opt(opt.substr(2));
    else if (opt.starts_with("-"))
      args = parse_shortform_opt(opt.substr(1), args);
  }
}

void cli_parser::parse(int argc, const char** argv) {
  parse(arg_span(argv, argc));
}

const std::string& cli_parser::program_name() const noexcept {
  return program_name_;
}

bool cli_parser::contains_option(const std::string& id) const {
  return parsed_.contains(id);
}

std::size_t cli_parser::num_option_values(const std::string& id) const {
  return parsed_.at(id).size();
}

bool cli_parser::has_option_value(const std::string& id) const {
  return num_option_values(id) != 0;
}

const cli_parser::string_list&
cli_parser::option_values(const std::string& id) const {
  return parsed_.at(id);
}

// TODO: Use equals sign to separate longform
void cli_parser::parse_longform_opt(std::string_view longform) {
  const auto pos = std::find_if(options_.begin(), options_.end(),
                                [longform](const option& opt) {
                                  return longform.starts_with(opt.longform_);
                                });
  if (pos == options_.end())
    return;
  parsed_.emplace(pos->id_, string_list{});

  if (pos->has_value_) {
    auto parts = util::split(std::string{longform}, '=');
    if (parts.size() != 2)
      throw std::runtime_error(
        "Missing an expected argument for longform option \""s
        + std::string(longform) + "\"");
    parsed_.at(pos->id_).emplace_back(std::move(parts.back()));
  }
}

// TODO: allow chaining shortform options (-a -b -c -d -e -> -abcde)
cli_parser::arg_span cli_parser::parse_shortform_opt(std::string_view opt,
                                                     arg_span args) {
  for (auto c = opt.front(); !opt.empty();
       opt = opt.substr(1), c = opt.front()) {
    const auto pos = std::find(options_.begin(), options_.end(), c);
    if (pos == options_.end())
      return args;
    parsed_.emplace(pos->id_, string_list{});

    if (pos->has_value_) {
      if (opt.size() > 1) {
        // Treating the following chars as the argument value
        parsed_.at(pos->id_).emplace_back(opt.substr(1));
      } else if (opt.size() == 1) {
        // an argument may have been specified with whitespace (-o option)
        args = args.subspan(1);
        if (args.empty() || std::string_view{args.front()}.starts_with("-"))
          throw std::runtime_error(
            "Missing an expected argument for shortform option'"s + c + "'");
        parsed_.at(pos->id_).emplace_back(args.front());
      }
    }
  }

  return args;
}

} // namespace util
