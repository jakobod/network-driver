/**
 *  @author    Jakob Otto
 *  @file      cli_parser.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
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

/// @brief Parser for CLI arguments.
class cli_parser {
public:
  // -- Public member types ----------------------------------------------------

  /// @brief Option representation for registering options within the parser
  struct option {
    /// @brief Constructs an option instance
    /// @param id  Unique id of the option
    /// @param longform  Longform (--...) of the cli option
    /// @param shortform  Shortform (-...) of the cli option
    /// @param has_value  Wether an argument value is expected
    option(std::string id, std::string longform, char shortform,
           bool has_value);

    /// @brief Unique id of the option
    const std::string id_;
    /// @brief Longform of the option
    const std::string longform_;
    /// @brief Shortform of the option
    const char shortform_;
    /// @brief Wether an argument value is expected
    const bool has_value_;

    /// @brief Compares two options for equality
    /// @param other  The other instance to compare to
    /// @returns true in case both objects are equal, false otherwise
    bool operator==(const option& other) const noexcept;

    /// @brief Compares an option to a longform option
    /// @param longform  The longform to compare to
    /// @returns true in case the option has the longform, false otherwise
    bool operator==(const std::string_view& longform) const noexcept;

    /// @brief Compares an option to a shortform option
    /// @param longform  The shortform to compare to
    /// @returns true in case the option has the shortform, false otherwise
    bool operator==(char shortform) const noexcept;

    /// @brief Checks wether this object is less than `other`
    /// @param other  The other instance to compare to
    /// @returns true in case this object is less, false otherwise
    bool operator<(const option& other) const noexcept;

    /// @brief Checks wether this object is more than `other`
    /// @param other  The other instance to compare to
    /// @returns true in case this object is more, false otherwise
    bool operator>(const option& other) const noexcept;
  };

  /// @brief Type of the argument span
  using arg_span = std::span<const char*>;

private:
  // -- Private member types ---------------------------------------------------

  /// @brief A set of options checking for equality instead of sorting
  using option_set = std::set<option, std::not_equal_to<>>;
  /// @brief A list of strings
  using string_list = std::vector<std::string>;
  /// @brief Map of {id, [arguments...]}
  using result_map = std::map<std::string, string_list>;

public:
  // -- Public member constants ------------------------------------------------

  /// @brief Denotes a missing longform option
  static constexpr const char* no_longform = "";
  /// @brief Denotes a missing shortform option
  static constexpr const char no_shortform = '\0';

  // -- Adding and parsing options ---------------------------------------------

  /// @brief Registers an option in the parser
  /// @param opt The option to register in the parser
  /// @returns a reference to the parser
  cli_parser& register_option(option opt);

  /// @brief Constructs and registers an option in the parser
  /// @tparam ...Ts  Option constructor Argument types
  /// @param ...xs  Arguments to forward to the option constructor
  /// @return a reference to the parser
  template <class... Ts>
  cli_parser& register_option(Ts&&... xs) {
    return register_option(option{std::forward<Ts>(xs)...});
  }

  /// @brief Parses the specified cli aruments using the previously registered
  ///        options
  /// @param args  The arguments to parse
  void parse(arg_span args);

  /// @brief Parses the specified cli aruments using the previously registered
  ///        options
  /// @param argc The number of arguments to parse
  /// @param argv The argument values to parse
  void parse(int argc, const char** argv);

  // -- Queries ----------------------------------------------------------------

  /// @brief Returns the name of the program
  /// @return the name of the program
  const std::string& program_name() const noexcept;

  /// @brief Queries the parser wether an option with `id` exists in the args
  /// @param id the id of the option in question
  /// @returns true if an option with `id` was specified, false otherwise
  bool contains_option(const std::string& id) const;

  /// @brief Returns the number of arguments for `id`
  /// @param id the `id` for which to query the parser
  /// @returns the number of arguments
  std::size_t num_option_values(const std::string& id) const;

  /// @brief Checks wether an option has been specified
  /// @param id The `id` of the option
  /// @returns true if the option has been specified, false otherwise
  bool has_option_value(const std::string& id) const;

  /// @brief Returns a list of all arguments for the option with `id`
  /// @param id The `id` of the option
  /// @returns a reference to the list of all arguments as strings
  const string_list& option_values(const std::string& id) const;

  // -- Option value access/conversion -----------------------------------------

  /// @brief Template definition for the partial template specialization of the
  ///        conversion functions
  /// @tparam T  Type to convert to
  /// @param id  the `id` of the option for which to obtain the argument
  /// @param pos  the index of the argument
  /// @throws std::runtime_error in case the specified `T` is not supported
  /// @returns default constructed T instance
  template <class T>
  T option_value([[maybe_unused]] const std::string& id,
                 [[maybe_unused]] std::size_t pos = 0) const {
    throw std::runtime_error{"Conversion to specified type is not supported"};
    return {};
  }

  /// @brief Partial template specialization for converting to std::string
  /// @param id  the `id` of the option for which to obtain the argument
  /// @param pos  the index of the argument
  /// @throws std::out_of_range in case `pos` is out of bounds
  /// @returns An argument value as std::string
  template <>
  std::string option_value(const std::string& id, std::size_t pos) const {
    return parsed_.at(id).at(pos);
  }

  /// @brief Partial template specialization for converting to int
  /// @param id  the `id` of the option for which to obtain the argument
  /// @param pos  the index of the argument
  /// @throws std::out_of_range in case `pos` is out of bounds or the conversion
  ///         would exceed the bounds of int
  /// @throws std::invalid_argument if no conversion could be performed
  /// @returns An argument value as int
  template <>
  int option_value(const std::string& id, std::size_t pos) const {
    return std::stoi(parsed_.at(id).at(pos));
  }

  /// @brief Partial template specialization for converting to std::size_t
  /// @param id  the `id` of the option for which to obtain the argument
  /// @param pos  the index of the argument
  /// @throws std::out_of_range in case `pos` is out of bounds or the conversion
  ///         would exceed the bounds of std::size_t
  /// @throws std::invalid_argument if no conversion could be performed
  /// @returns An argument value as std::string
  template <>
  std::size_t option_value(const std::string& id, std::size_t pos) const {
    return std::stoul(parsed_.at(id).at(pos));
  }

  /// @brief Partial template specialization for converting to std::int64_t
  /// @param id  the `id` of the option for which to obtain the argument
  /// @param pos  the index of the argument
  /// @throws std::out_of_range in case `pos` is out of bounds or the conversion
  ///         would exceed the bounds of std::int64_t
  /// @throws std::invalid_argument if no conversion could be performed
  /// @returns An argument value as std::int64_t
  template <>
  std::int64_t option_value(const std::string& id, std::size_t pos) const {
    return std::stoll(parsed_.at(id).at(pos));
  }

  /// @brief Partial template specialization for converting to float
  /// @param id  the `id` of the option for which to obtain the argument
  /// @param pos  the index of the argument
  /// @throws std::out_of_range in case `pos` is out of bounds or the conversion
  ///         would exceed the bounds of float
  /// @throws std::invalid_argument if no conversion could be performed
  /// @returns An argument value as float
  template <>
  float option_value(const std::string& id, std::size_t pos) const {
    return std::stof(parsed_.at(id).at(pos));
  }

  /// @brief Partial template specialization for converting to double
  /// @param id  the `id` of the option for which to obtain the argument
  /// @param pos  the index of the argument
  /// @throws std::out_of_range in case `pos` is out of bounds or the conversion
  ///         would exceed the bounds of double
  /// @throws std::invalid_argument if no conversion could be performed
  /// @returns An argument value as double
  template <>
  double option_value(const std::string& id, std::size_t pos) const {
    return std::stod(parsed_.at(id).at(pos));
  }

  /// @brief Template for converting the argument value of option with `id` at
  ///        `pos` using the specified conversion function `conv`
  /// @tparam ConversionFunc  Type of `conv`
  /// @param id  the `id` of the option for which to obtain and convert the
  ///        argument
  /// @param conv  the conversion function to convert the argument with
  /// @param pos  the index of the argument
  /// @returns the conversion result of the conversion function `conv`
  template <class ConversionFunc>
  auto option_value(const std::string& id, ConversionFunc conv,
                    std::size_t pos = 0) const {
    return conv(parsed_.at(id).at(pos));
  }

private:
  /// @brief Parses a longform option with or without arguments
  /// @param opt the option to parse
  void parse_longform_opt(std::string_view opt);

  /// @brief Parses a single or multiple shortform option with or without
  /// arguments
  /// @param opt the option(s) in question
  /// @param args  the remaining arguments, needed for retrieving option values
  /// @returns  the option values after parsing possible option values
  arg_span parse_shortform_opt(std::string_view opt, arg_span args);

  /// @brief The registered options
  option_set options_;
  /// @brief The parsed program name
  std::string program_name_;
  /// @brief The parsed options and possible values
  result_map parsed_;
};

} // namespace util
