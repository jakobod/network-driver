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
  static constexpr const std::string_view esc_seq_reset{"\033[0m"};
  static constexpr const std::string_view esc_seq_bold{"\033[1m"};
  static constexpr const std::string_view esc_seq_reset_bold{"\033[22m"};
  static constexpr const std::string_view esc_seq_red{"\033[31m"};
  static constexpr const std::string_view esc_seq_green{"\033[32m"};
  static constexpr const std::string_view esc_seq_yellow{"\033[33m"};

  struct log_level {};

public:
  struct debug : public log_level {};
  struct info : public debug {};
  struct warning : public info {};
  struct error : public warning {};

  template <meta::derived_from<log_level> Level, class... Ts>
  static void
  log(const std::string& file_name, const int line, const Ts&... ts) {
    const std::string location = (file_name + ":" + std::to_string(line));
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
  template <class T>
  static void append_to_stream(const T& t) {
    std::cout << t;
  }

  template <class... Ts>
  static void log_debug(const std::string& location, const Ts&... ts) {
    std::cout << esc_seq_bold << "[Debug]   " << location << " - "
              << esc_seq_reset_bold << (... << ts) << std::endl;
  }

  template <class... Ts>
  static void log_info(const std::string& location, const Ts&... ts) {
    std::cout << esc_seq_green << esc_seq_bold << "[Info]    " << location
              << " - " << esc_seq_reset_bold << (... << ts) << esc_seq_reset
              << std::endl;
  }

  template <class... Ts>
  static void log_warning(const std::string& location, const Ts&... ts) {
    std::cout << esc_seq_yellow << esc_seq_bold << "[Warning] " << location
              << " - " << esc_seq_reset_bold << (... << ts) << esc_seq_reset
              << std::endl;
  }

  template <class... Ts>
  static void log_error(const std::string& location, const Ts&... ts) {
    std::cout << esc_seq_red << esc_seq_bold << "[Error]   " << location
              << " - " << esc_seq_reset_bold << (... << ts) << esc_seq_reset
              << std::endl;
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
