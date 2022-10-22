/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#if defined(NET_LOG_LEVEL)

#  include "meta/concepts.hpp"
#  include "util/error.hpp"

#  include <fstream>
#  include <iostream>
#  include <sstream>
#  include <string>
#  include <string_view>

namespace util {

class logger {
  /// Escape sequences for formatting the logging output
  static constexpr const std::string_view reset_formatting{"\033[0m"};
  static constexpr const std::string_view reset_bold{"\033[22m"};
  static constexpr const std::string_view debug_formatting{"\033[1m"};
  static constexpr const std::string_view info_formatting{"\033[1;32m"};
  static constexpr const std::string_view warning_formatting{"\033[1;33m"};
  static constexpr const std::string_view error_formatting{"\033[1;31m"};

  struct log_level {};

public:
  struct debug : public log_level {};
  struct info : public debug {};
  struct warning : public info {};
  struct error : public warning {};

  using configured_log_level = NET_LOG_LEVEL;

  struct config {
    bool terminal_logging{true};
    std::string log_file_name;
  };

  static logger& instance() {
    static logger instance;
    return instance;
  }

  static util::error init(const config& cfg) {
    auto& inst = instance();
    if (!cfg.log_file_name.empty()) {
      inst.log_file_.open(cfg.log_file_name);
      if (!inst.log_file_.is_open())
        return {error_code::runtime_error,
                "Could not open log file with name: {0}", cfg.log_file_name};
    }
    inst.terminal_logging_ = cfg.terminal_logging;
    return none;
  }

  template <class... Ts>
  void log_debug(std::string_view location, const Ts&... ts) {
    if (terminal_logging_)
      std::cout << concatenate(debug_formatting, "[  Debug   ] ", location,
                               " - ", reset_bold, ts..., reset_formatting,
                               '\n');
    if (log_file_.is_open())
      log_file_ << concatenate("[  Debug   ] ", location, " - ", ts..., '\n');
  }

  template <class... Ts>
  void log_info(std::string_view location, const Ts&... ts) {
    if (terminal_logging_)
      std::cout << concatenate(info_formatting, "[   Info   ] ", location,
                               " - ", reset_bold, ts..., reset_formatting,
                               '\n');
    if (log_file_.is_open())
      log_file_ << concatenate("[   Info   ] ", location, " - ", ts..., '\n');
  }

  template <class... Ts>
  void log_warning(std::string_view location, const Ts&... ts) {
    if (terminal_logging_)
      std::cout << concatenate(warning_formatting, "[ Warning  ] ", location,
                               " - ", reset_bold, ts..., reset_formatting,
                               '\n');
    if (log_file_.is_open())
      log_file_ << concatenate("[ Warning  ] ", location, " - ", ts..., '\n');
  }

  template <class... Ts>
  void log_error(std::string_view location, const Ts&... ts) {
    if (terminal_logging_)
      std::cout << concatenate(error_formatting, "[  Error   ] ", location,
                               " - ", reset_bold, ts..., reset_formatting,
                               '\n');
    if (log_file_.is_open())
      log_file_ << concatenate("[  Error   ] ", location, " - ", ts..., '\n');
  }

private:
  logger() = default;

  template <class... Ts>
  std::string concatenate(const Ts&... ts) {
    // This approach using a stringstream is not very performant, but will
    // keep multiple threads from interleaving lines.
    std::stringstream ss;
    (ss << ... << ts);
    return ss.str();
  }

  bool terminal_logging_{true};
  std::ofstream log_file_;
};

} // namespace util

// utility macros for stringifying macro arguments
#  define STRINGIFY(x) STRINGIFY2(x)
#  define STRINGIFY2(x) #x

#  define LOG_DEBUG(...)                                                       \
    if constexpr (meta::derived_or_same_as<util::logger::configured_log_level, \
                                           util::logger::debug>)               \
      util::logger::instance().log_debug(__FILE__ ":" STRINGIFY(__LINE__),     \
                                         __VA_ARGS__);

#  define LOG_INFO(...)                                                        \
    if constexpr (meta::derived_or_same_as<util::logger::configured_log_level, \
                                           util::logger::info>)                \
      util::logger::instance().log_info(__FILE__ ":" STRINGIFY(__LINE__),      \
                                        __VA_ARGS__);

#  define LOG_WARNING(...)                                                     \
    if constexpr (meta::derived_or_same_as<util::logger::configured_log_level, \
                                           util::logger::warning>)             \
      util::logger::instance().log_warning(__FILE__ ":" STRINGIFY(__LINE__),   \
                                           __VA_ARGS__);

#  define LOG_ERROR(...)                                                       \
    if constexpr (meta::derived_or_same_as<util::logger::configured_log_level, \
                                           util::logger::error>)               \
      util::logger::instance().log_error(__FILE__ ":" STRINGIFY(__LINE__),     \
                                         __VA_ARGS__);

#else

#  define LOG_INFO(...)

#  define LOG_WARNING(...)

#  define LOG_DEBUG(...)

#  define LOG_ERROR(...)

#endif
