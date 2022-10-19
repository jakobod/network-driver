/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#if defined(NET_LOG_LEVEL)

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

  using configured_log_level = NET_LOG_LEVEL;

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

private:
  template <class... Ts>
  static void log_line(const Ts&... ts) {
    (std::cout << ... << ts) << std::endl;
  }
};

} // namespace util

#  define LOG_DEBUG(...)                                                       \
    if constexpr (meta::derived_or_same_as<util::logger::configured_log_level, \
                                           util::logger::debug>)               \
      util::logger::log_debug(                                                 \
        std::string(__FILE__) + ":" + std::to_string(__LINE__), __VA_ARGS__);

#  define LOG_INFO(...)                                                        \
    if constexpr (meta::derived_or_same_as<util::logger::configured_log_level, \
                                           util::logger::info>)                \
      util::logger::log_info(                                                  \
        std::string(__FILE__) + ":" + std::to_string(__LINE__), __VA_ARGS__);

#  define LOG_WARNING(...)                                                     \
    if constexpr (meta::derived_or_same_as<util::logger::configured_log_level, \
                                           util::logger::warning>)             \
      util::logger::log_warning(                                               \
        std::string(__FILE__) + ":" + std::to_string(__LINE__), __VA_ARGS__);

#  define LOG_ERROR(...)                                                       \
    if constexpr (meta::derived_or_same_as<util::logger::configured_log_level, \
                                           util::logger::error>)               \
      util::logger::log_error(                                                 \
        std::string(__FILE__) + ":" + std::to_string(__LINE__), __VA_ARGS__);

#else

#  define LOG_INFO(...)

#  define LOG_WARNING(...)

#  define LOG_DEBUG(...)

#  define LOG_ERROR(...)

#endif
