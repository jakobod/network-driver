/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#define NET_LOG_LEVEL_NONE (0)
#define NET_LOG_LEVEL_ERROR (1)
#define NET_LOG_LEVEL_WARNING (2)
#define NET_LOG_LEVEL_DEBUG (3)
#define NET_LOG_LEVEL_TRACE (4)

#if NET_LOG_LEVEL > NET_LOG_LEVEL_NONE

#  include "meta/concepts.hpp"
#  include "util/error.hpp"
#  include "util/scope_guard.hpp"

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
  static constexpr const std::string_view trace_formatting{"\033[1;34m"};
  static constexpr const std::string_view debug_formatting{"\033[1;32m"};
  static constexpr const std::string_view warning_formatting{"\033[1;33m"};
  static constexpr const std::string_view error_formatting{"\033[1;31m"};

public:
  static logger& instance() {
    static logger instance;
    return instance;
  }

  static void init(const bool terminal_logging,
                   const std::string_view log_file_path) {
    auto& inst = instance();
    if (!log_file_path.empty())
      inst.log_file_.open(log_file_path);
    inst.terminal_logging_ = terminal_logging;
  }

  template <class... Ts>
  void log_trace(std::string_view func_name, const Ts&... ts) {
    log(trace_formatting, "[TRACE]", func_name, ts...);
  }

  template <class... Ts>
  void log_debug(std::string_view func_name, const Ts&... ts) {
    log(debug_formatting, "[DEBUG]", func_name, ts...);
  }

  template <class... Ts>
  void log_warning(std::string_view func_name, const Ts&... ts) {
    log(warning_formatting, "[WARNING]", func_name, ts...);
  }

  template <class... Ts>
  void log_error(std::string_view func_name, const Ts&... ts) {
    log(error_formatting, "[ERROR]", func_name, ts...);
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

  template <class... Ts>
  void log(std::string_view formatting, std::string_view level,
           std::string_view func_name, const Ts&... ts) {
    if (terminal_logging_)
      std::cout << concatenate(formatting, level, " ", func_name, " - ",
                               reset_bold, ts..., reset_formatting, '\n');
    if (log_file_.is_open())
      log_file_ << concatenate(level, " ", func_name, " - ", ts..., '\n');
  }

  bool terminal_logging_{true};
  std::ofstream log_file_;
};

} // namespace util

/// Macro for initializing the logger
#  define LOG_INIT(terminal_logging, log_file_path)                            \
    util::logger::init(terminal_logging, log_file_path)

/// Macro for tracing runtime paths, prints entry and exit messages
#  if NET_LOG_LEVEL >= NET_LOG_LEVEL_TRACE
#    define LOG_TRACE()                                                        \
      util::logger::instance().log_trace(__PRETTY_FUNCTION__, "Entry");        \
      util::scope_guard guard{[func_name = __PRETTY_FUNCTION__] {              \
        util::logger::instance().log_trace(func_name, "Exit");                 \
      }};
#  endif

/// Macro for prints debug messages
#  if NET_LOG_LEVEL >= NET_LOG_LEVEL_DEBUG
#    define LOG_DEBUG(...)                                                     \
      util::logger::instance().log_debug(__PRETTY_FUNCTION__, __VA_ARGS__)
#  endif

/// Macro for prints warning messages
#  if NET_LOG_LEVEL >= NET_LOG_LEVEL_WARNING
#    define LOG_WARNING(...)                                                   \
      util::logger::instance().log_warning(__PRETTY_FUNCTION__, __VA_ARGS__)
#  endif

/// Macro for prints error messages
#  if NET_LOG_LEVEL >= NET_LOG_LEVEL_ERROR
#    define LOG_ERROR(...)                                                     \
      util::logger::instance().log_error(__PRETTY_FUNCTION__, __VA_ARGS__)
#  endif

#endif

// -- Anything that has not been defined shall be default defined --------------

#ifndef LOG_INIT
#  define LOG_INIT(...)
#endif

#ifndef LOG_TRACE
#  define LOG_TRACE(...)
#endif

#ifndef LOG_DEBUG
#  define LOG_DEBUG(...)
#endif

#ifndef LOG_WARNING
#  define LOG_WARNING(...)
#endif

#ifndef LOG_ERROR
#  define LOG_ERROR(...)
#endif
