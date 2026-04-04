/**
 *  @author    Jakob Otto
 *  @file      cli_parser.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <ostream>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace util {

/// @brief Command-line argument parser for structured option handling.
/// Provides registration of options with optional values and parsing of
/// command-line arguments in both long form (--option) and short form (-o)
/// styles. Supports querying parsed options and converting argument values to
/// specific types.
class cli_parser {
public:
  // -- Public member types ----------------------------------------------------

  /// @brief Representation of a command-line option.
  struct option {
    /// @brief Constructs an option with all parameters.
    /// @param id Unique identifier for the option.
    /// @param longform The long form of the option (e.g., "verbose").
    /// @param shortform The short form of the option (e.g., 'v').
    /// @param has_value Whether this option expects a value argument.
    option(std::string id, std::string longform, char shortform,
           bool has_value);

    /// @brief Unique identifier for this option.
    const std::string id_;
    /// @brief Long form of the option (without dashes).
    const std::string longform_;
    /// @brief Short form of the option (single character).
    const char shortform_;
    /// @brief Whether an argument value is expected.
    const bool has_value_;

    /// @brief Equality comparison with another option.
    /// @param other The option to compare with.
    /// @return True if both options are identical.
    bool operator==(const option& other) const noexcept;

    /// @brief Equality comparison with a longform string.
    /// @param longform The long form to compare with.
    /// @return True if this option has the specified long form.
    bool operator==(const std::string_view& longform) const noexcept;

    /// @brief Equality comparison with a shortform character.
    /// @param shortform The character to compare with.
    /// @return True if this option has the specified short form.
    bool operator==(char shortform) const noexcept;

    /// @brief Less-than comparison with another option.
    /// @param other The option to compare with.
    /// @return True if this option is less than the other.
    bool operator<(const option& other) const noexcept;

    /// @brief Greater-than comparison with another option.
    /// @param other The option to compare with.
    /// @return True if this option is greater than the other.
    bool operator>(const option& other) const noexcept;
  };

  /// @brief Type alias for span of command-line arguments.
  using arg_span = std::span<const char*>;

private:
  // -- Private member types ---------------------------------------------------

  /// @brief A set of options with equality-based comparison.
  using option_set = std::set<option, std::not_equal_to<>>;
  /// @brief A vector of strings for argument values.
  using string_list = std::vector<std::string>;
  /// @brief Map from option ID to argument values.
  using result_map = std::map<std::string, string_list>;

public:
  // -- Public member constants ------------------------------------------------

  /// @brief Constant indicating no long form option is available.
  static constexpr const char* no_longform = "";
  /// @brief Constant indicating no short form option is available.
  static constexpr const char no_shortform = '\0';

  // -- Adding and parsing options ---------------------------------------------

  /// @brief Registers an option in the parser.
  /// @param opt The option to register.
  /// @return Reference to this parser for method chaining.
  cli_parser& register_option(option opt);

  /// @brief Constructs and registers an option in the parser.
  /// @tparam Ts Types of arguments for the option constructor.
  /// @param xs Arguments to forward to the option constructor.
  /// @return Reference to this parser for method chaining.
  template <class... Ts>
  cli_parser& register_option(Ts&&... xs) {
    return register_option(option{std::forward<Ts>(xs)...});
  }

  /// @brief Parses command-line arguments using registered options.
  /// @param args The arguments to parse (typically argv without program name).
  void parse(arg_span args);

  /// @brief Parses command-line arguments using argc/argv format.
  /// @param argc The number of arguments.
  /// @param argv The argument values (including program name at argv[0]).
  void parse(int argc, const char** argv);

  // -- Queries ----------------------------------------------------------------

  /// @brief Returns the name of the parsed program.
  /// Extracted from argv[0] during parsing.
  /// @return The program name as a string.
  const std::string& program_name() const noexcept;

  /// @brief Checks if an option with the specified ID exists in parsed
  /// arguments.
  /// @param id The unique ID of the option.
  /// @return True if the option was specified on the command line.
  bool contains_option(const std::string& id) const;

  /// @brief Returns the number of argument values for an option.
  /// @param id The unique ID of the option.
  /// @return The count of values provided for this option.
  std::size_t num_option_values(const std::string& id) const;

  /// @brief Checks whether an option has at least one argument value.
  /// @param id The unique ID of the option.
  /// @return True if the option has at least one value.
  bool has_option_value(const std::string& id) const;

  /// @brief Returns all argument values for an option.
  /// @param id The unique ID of the option.
  /// @return A const reference to the list of argument strings.
  const string_list& option_values(const std::string& id) const;

  // -- Option value access/conversion -----------------------------------------

  /// @brief Generic template for converting option values.
  /// Specializations below provide actual implementations for supported types.
  /// @tparam T The type to convert the argument value to.
  /// @param id The unique ID of the option.
  /// @param pos The index of the argument (default: 0 for first value).
  /// @throws std::runtime_error If type T is not supported.
  /// @return A default-constructed instance of T (base template).
  template <class T>
  inline T option_value([[maybe_unused]] const std::string& id,
                        [[maybe_unused]] std::size_t pos = 0) const {
    throw std::runtime_error{"Conversion to specified type is not supported"};
    return {};
  }

  /// @brief Converts an option value using a custom conversion function.
  /// Allows flexible conversion to types not directly supported.
  /// @tparam ConversionFunc The type of the conversion function.
  /// @param id The unique ID of the option.
  /// @param conv The conversion function to apply to the string value.
  /// @param pos The index of the argument (default: 0 for first value).
  /// @return The result of applying conv to the argument value.
  template <class ConversionFunc>
  auto option_value(const std::string& id, ConversionFunc conv,
                    std::size_t pos = 0) const {
    return conv(parsed_.at(id).at(pos));
  }

private:
  /// @brief Parses a long-form option (--option or --option=value).
  /// @param opt The long-form option string to parse.
  void parse_longform_opt(std::string_view opt);

  /// @brief Parses one or more short-form options (-o or -abc).
  /// Handles combined short options and their values.
  /// @param opt The short-form option string(s) to parse.
  /// @param args The remaining arguments (may be consumed for option values).
  /// @return The remaining arguments after parsing.
  arg_span parse_shortform_opt(std::string_view opt, arg_span args);

  /// @brief The set of registered options.
  option_set options_;
  /// @brief The program name extracted from argv[0].
  std::string program_name_;
  /// @brief Map of parsed option IDs to their argument values.
  result_map parsed_;
};

// -- Template specializations for option_value (at namespace scope) -----------

/// @brief Template specialization for std::string option values.
/// Returns the argument value as-is, without conversion.
/// @param id The unique ID of the option.
/// @param pos The index of the argument (default: 0).
/// @throws std::out_of_range If pos is out of bounds.
/// @return The argument value as a string.
template <>
inline std::string
cli_parser::option_value<std::string>(const std::string& id,
                                      std::size_t pos) const {
  return parsed_.at(id).at(pos);
}

/// @brief Template specialization for int option values.
/// Converts the string argument to an integer.
/// @param id The unique ID of the option.
/// @param pos The index of the argument (default: 0).
/// @throws std::out_of_range If pos is out of bounds.
/// @throws std::invalid_argument If the value cannot be converted to int.
/// @return The argument value as an int.
template <>
inline int
cli_parser::option_value<int>(const std::string& id, std::size_t pos) const {
  return std::stoi(parsed_.at(id).at(pos));
}

/// @brief Partial template specialization for converting to std::size_t
/// @param id  the `id` of the option for which to obtain the argument
/// @param pos  the index of the argument
/// @throws std::out_of_range in case `pos` is out of bounds or the conversion
///         would exceed the bounds of std::size_t
/// @throws std::invalid_argument if no conversion could be performed
/// @returns An argument value as std::size_t
template <>
inline std::size_t
cli_parser::option_value<std::size_t>(const std::string& id,
                                      std::size_t pos) const {
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
inline std::int64_t
cli_parser::option_value<std::int64_t>(const std::string& id,
                                       std::size_t pos) const {
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
inline float
cli_parser::option_value<float>(const std::string& id, std::size_t pos) const {
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
inline double
cli_parser::option_value<double>(const std::string& id, std::size_t pos) const {
  return std::stod(parsed_.at(id).at(pos));
}

} // namespace util
