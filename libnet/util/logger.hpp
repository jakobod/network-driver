/**
 *  @author    Jakob Otto
 *  @file      logger.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

/// @brief Logging level constant: no logging output.
#define NET_LOG_LEVEL_NONE (0)
/// @brief Logging level constant: only error messages.
#define NET_LOG_LEVEL_ERROR (1)
/// @brief Logging level constant: error and warning messages.
#define NET_LOG_LEVEL_WARNING (2)
/// @brief Logging level constant: error, warning, and debug messages.
#define NET_LOG_LEVEL_DEBUG (3)
/// @brief Logging level constant: all messages including trace.
#define NET_LOG_LEVEL_TRACE (4)

#if NET_LOG_LEVEL > NET_LOG_LEVEL_NONE

#  include "meta/concepts.hpp"

#  include "util/config.hpp"
#  include "util/scope_guard.hpp"

#  include <cstdint>
#  include <fstream>
#  include <iostream>
#  include <sstream>
#  include <string>
#  include <string_view>

namespace util {

/// @brief Thread-safe logging facility for runtime diagnostics.
/// Provides compile-time configurable logging with support for console
/// and file output, indentation tracking, and color formatting.
/// Use the LOG_TRACE, LOG_DEBUG, LOG_WARNING, LOG_ERROR macros to log messages.
class logger {
  static constexpr const std::string_view config_log_file_option
    = "logger.file-path";
  static constexpr const std::string_view config_terminal_output_option
    = "logger.terminal-output";
  /// @brief ANSI escape sequence for resetting formatting.
  static constexpr const std::string_view reset_formatting = "\033[0m";
  /// @brief ANSI escape sequence for resetting bold text.
  static constexpr const std::string_view reset_bold = "\033[22m";
  /// @brief ANSI escape sequence for blue bold text (trace level).
  static constexpr const std::string_view trace_formatting = "\033[1;34m";
  /// @brief ANSI escape sequence for green bold text (debug level).
  static constexpr const std::string_view debug_formatting = "\033[1;32m";
  /// @brief ANSI escape sequence for yellow bold text (warning level).
  static constexpr const std::string_view warning_formatting = "\033[1;33m";
  /// @brief ANSI escape sequence for red bold text (error level).
  static constexpr const std::string_view error_formatting = "\033[1;31m";

public:
  /// @brief Wrapper for logging arguments that pairs names with values.
  /// Allows logging in the format "name=value" for readability.
  template <class T>
  struct arg_wrapper {
    /// @brief Constructs an argument wrapper.
    /// @param name The name of the argument.
    /// @param t The value of the argument.
    arg_wrapper(std::string_view name, T t) : name_{name}, t_{std::move(t)} {}

    /// @brief Stream output operator for the wrapper.
    friend std::ostream& operator<<(std::ostream& os,
                                    const arg_wrapper<T>& wrapper) {
      return os << wrapper.name_ << "=" << wrapper.t_;
    }

  public:
    const std::string_view name_; ///< The argument name.
    const T t_;                   ///< The argument value.
  };

  /// @brief Retrieves the global logger instance (singleton).
  /// @return Reference to the unique logger instance.
  static logger& instance() {
    static logger instance;
    return instance;
  }

  /// @brief Initializes the logger with configuration options.
  /// Sets up file logging path and terminal output settings from config.
  /// @param cfg The configuration object containing logger options.
  static void init(const config& cfg) {
    if (const auto file_path
        = cfg.get<std::string>(config_log_file_option.data())) {
      std::cout << "opening log_file \"" << *file_path << "\"" << std::endl;
      instance().log_file_.open(*file_path);
    }
    instance().terminal_logging_
      = cfg.get_or(config_terminal_output_option.data(), false);
  }

  /// @brief Logs a trace-level message.
  /// @tparam Ts Types of the arguments to log.
  /// @param func_name The name of the function (typically __PRETTY_FUNCTION__).
  /// @param ts The arguments to log.
  template <class... Ts>
  void log_trace(std::string_view func_name, const Ts&... ts) {
    log(trace_formatting, "[TRACE]  ", func_name, ts...);
  }

  /// @brief Logs a debug-level message.
  /// @tparam Ts Types of the arguments to log.
  /// @param func_name The name of the function (typically __PRETTY_FUNCTION__).
  /// @param ts The arguments to log.
  template <class... Ts>
  void log_debug(std::string_view func_name, const Ts&... ts) {
    log(debug_formatting, "[DEBUG]  ", func_name, ts...);
  }

  /// @brief Logs a warning-level message.
  /// @tparam Ts Types of the arguments to log.
  /// @param func_name The name of the function (typically __PRETTY_FUNCTION__).
  /// @param ts The arguments to log.
  template <class... Ts>
  void log_warning(std::string_view func_name, const Ts&... ts) {
    log(warning_formatting, "[WARNING]", func_name, ts...);
  }

  /// @brief Logs an error-level message.
  /// @tparam Ts Types of the arguments to log.
  /// @param func_name The name of the function (typically __PRETTY_FUNCTION__).
  /// @param ts The arguments to log.
  template <class... Ts>
  void log_error(std::string_view func_name, const Ts&... ts) {
    log(error_formatting, "[ERROR]  ", func_name, ts...);
  }

  /// @brief Increases the logging indentation level.
  /// Used to visually nest related log messages.
  void increase_indent() noexcept { indent_ += "  "; }

  /// @brief Decreases the logging indentation level.
  /// Undoes a previous call to increase_indent().
  void decrease_indent() noexcept { indent_.resize(indent_.size() - 2); }

private:
  /// @brief Private default constructor.
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
    if (terminal_logging_) {
      std::cout << concatenate(indent_, formatting, level, " ", func_name,
                               " - ", reset_bold, ts..., reset_formatting,
                               '\n');
    }
    if (log_file_.is_open()) {
      log_file_ << concatenate(indent_, level, " ", func_name, " - ", ts...,
                               '\n');
    }
  }

  bool terminal_logging_ = false; ///< Whether to log to terminal.
  std::ofstream log_file_;        ///< The log file output stream.
  std::string indent_;            ///< Current indentation string.
};

} // namespace util

/// @brief Creates a named argument wrapper for logging, e.g., NET_ARG(x)
/// produces "x=<value>".
#  define NET_ARG(arg) util::logger::arg_wrapper(#arg, arg)

/// @brief Creates an explicitly named argument wrapper, e.g., NET_ARG2("count",
/// x) produces "count=<value>".
#  define NET_ARG2(name, arg) util::logger::arg_wrapper(name, arg)

/// @brief Macro for initializing the logger with a configuration object.
#  define LOG_INIT(cfg) util::logger::init(cfg)

/// @brief Macro for tracing runtime execution paths.
/// Prints entry and exit messages with automatic indentation management.
#  if NET_LOG_LEVEL >= NET_LOG_LEVEL_TRACE
#    define LOG_TRACE()                                                        \
      util::logger::instance().log_trace(__PRETTY_FUNCTION__, ">>> ENTRY");    \
      util::logger::instance().increase_indent();                              \
      const util::scope_guard guard{[func_name = __PRETTY_FUNCTION__] {        \
        util::logger::instance().decrease_indent();                            \
        util::logger::instance().log_trace(func_name, "<<< EXIT");             \
      }};
#  endif

/// @brief Macro for logging debug-level messages.
/// @brief Macro for logging debug-level messages.
#  if NET_LOG_LEVEL >= NET_LOG_LEVEL_DEBUG
#    define LOG_DEBUG(...)                                                     \
      util::logger::instance().log_debug(__PRETTY_FUNCTION__, __VA_ARGS__)
#  endif

/// @brief Macro for logging warning-level messages.
#  if NET_LOG_LEVEL >= NET_LOG_LEVEL_WARNING
#    define LOG_WARNING(...)                                                   \
      util::logger::instance().log_warning(__PRETTY_FUNCTION__, __VA_ARGS__)
#  endif

/// @brief Macro for logging error-level messages.
#  if NET_LOG_LEVEL >= NET_LOG_LEVEL_ERROR
#    define LOG_ERROR(...)                                                     \
      util::logger::instance().log_error(__PRETTY_FUNCTION__, __VA_ARGS__)
#  endif

#endif

// -- Anything that has not been defined shall be default defined --------------

#ifndef NET_ARG
#  define NET_ARG(...)
#endif

#ifndef NET_ARG2
#  define NET_ARG2(...)
#endif

#ifndef NET_ARG2
#  define NET_ARG2(...)
#endif

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
