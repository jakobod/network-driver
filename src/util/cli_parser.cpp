/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "util/cli_parser.hpp"

#include <compare>
#include <iostream>
#include <span>

using option = util::cli_parser::option;

namespace {} // namespace

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
  return (id_ == other.id_) || (longform_ == other.longform_)
         || (shortform_ == other.shortform_);
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

cli_parser& cli_parser::add_option(option opt) {
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
    if (opt.starts_with("--"))
      args = parse_opt(opt.substr(2), args); // This is a span type
    else if (opt.starts_with("-"))
      args = parse_opt(opt.back(), args); // This should be only a char
    else
      std::cout << "Positional argument \"" << opt << "\" found" << std::endl;
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

cli_parser::string_list& cli_parser::option_values(const std::string& id) {
  return parsed_.at(id);
}

const cli_parser::string_list&
cli_parser::option_values(const std::string& id) const {
  return parsed_.at(id);
}

} // namespace util
