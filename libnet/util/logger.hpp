/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#if defined(NET_ENABLE_LOGGING)

#  include "meta/concepts.hpp"

#  include <fstream>
#  include <iostream>

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

  template <meta::derived_from<log_level> Level, class... Ts>
  static void
  log(const std::string& file_name, const std::size_t line, const Ts&... ts) {
    const std::string location = file_name + ":" + std::to_string(line);
    if constexpr (meta::same_as<Level, debug>) {
      log_debug(location, ts...);
    } else if constexpr (meta::same_as<Level, info>) {
      log_info(location, ts...);
    } else if constexpr (meta::same_as<Level, warning>) {
      log_warning(location, ts...);
    } else if constexpr (meta::same_as<Level, error>) {
      log_error(location, ts...);
    }
  }

private:
  template <class... Ts>
  static void log_line(const Ts&... ts) {
    (std::cout << ... << ts) << std::endl;
  }

  template <class... Ts>
  static void log_debug(const std::string& location, const Ts&... ts) {
    log_line(debug_formatting, "[Debug]   ", location, " - ", reset_bold, ts...,
             reset_formatting);
  }

  template <class... Ts>
  static void log_info(const std::string& location, const Ts&... ts) {
    log_line(info_formatting, "[Info]    ", location, " - ", reset_bold, ts...,
             reset_formatting);
  }

  template <class... Ts>
  static void log_warning(const std::string& location, const Ts&... ts) {
    log_line(warning_formatting, "[Warning] ", location, " - ", reset_bold,
             ts..., reset_formatting);
  }

  template <class... Ts>
  static void log_error(const std::string& location, const Ts&... ts) {
    log_line(error_formatting, "[Error]   ", location, " - ", reset_bold, ts...,
             reset_formatting);
  }
};

} // namespace util

#  define LOG_DEBUG(...)                                                       \
    util::logger::log<util::logger::debug>(__FILE__, __LINE__, __VA_ARGS__);

#  define LOG_INFO(...)                                                        \
    util::logger::log<util::logger::info>(__FILE__, __LINE__, __VA_ARGS__);

#  define LOG_WARNING(...)                                                     \
    util::logger::log<util::logger::warning>(__FILE__, __LINE__, __VA_ARGS__);

#  define LOG_ERROR(...)                                                       \
    util::logger::log<util::logger::error>(__FILE__, __LINE__, __VA_ARGS__);

#else

#  define LOG_INFO(...)

#  define LOG_WARNING(...)

#  define LOG_DEBUG(...)

#  define LOG_ERROR(...)

#endif
