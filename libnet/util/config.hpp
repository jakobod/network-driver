/**
 *  @author    Jakob Otto
 *  @file      config.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "meta/concepts.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>

namespace util {

/// @brief Configuration storage and management for runtime parameters.
/// Stores configuration key-value pairs, supporting bool, int64_t, double,
/// and std::string values. Can be populated from configuration files or
/// programmatically at runtime.
class config {
public:
  // -- Public member types ----------------------------------------------------

  /// @brief Type of the key used for mapping the entries
  using key_type = std::string;
  /// @brief Type of the entries (variant of bool, int64_t, double, std::string)
  using entry_type = std::variant<bool, std::int64_t, double, std::string>;
  /// @brief Type of the dictionary used for mapping the entries
  using dictionary = std::unordered_map<key_type, entry_type>;

  /// @brief Constructs an empty configuration instance.
  config() = default;

  /// @brief Constructs a configuration instance by parsing a config file.
  /// @param config_path Path to the configuration file.
  config(const std::string& config_path);

  /// @brief Copy constructor.
  /// @param other The configuration to copy.
  config(const config& other) = default;

  /// @brief Move constructor.
  /// @param other The configuration to move from.
  config(config&& other) = default;

  /// @brief Copy assignment operator.
  /// @param other The configuration to copy.
  /// @return Reference to this.
  config& operator=(const config& other) noexcept = default;

  /// @brief Move assignment operator.
  /// @param other The configuration to move from.
  /// @return Reference to this.
  config& operator=(config&& other) noexcept = default;

  /// @brief Parses a configuration file and adds entries to this instance.
  /// Reads the file and extracts configuration key-value pairs.
  /// @param config_path Path to the configuration file.
  void parse(const std::string& config_path);

  /// @brief Adds a single configuration entry.
  /// @tparam Entry The value type (must be bool, int64_t, double, or string).
  /// @param key The configuration key.
  /// @param entry The configuration value.
  template <meta::one_of<bool, std::int64_t, double, std::string> Entry>
  void add_config_entry(key_type key, Entry entry) {
    config_values_.emplace(std::move(key), std::move(entry));
  }

  /// @brief Checks if the configuration contains a specific typed entry.
  /// @tparam T The expected value type.
  /// @param key The configuration key.
  /// @return True if the configuration contains the key with the specified
  /// type.
  template <meta::one_of<bool, std::int64_t, double, std::string> T>
  bool has_entry(const key_type& key) const {
    if (!config_values_.contains(key)) {
      return false;
    }
    return std::holds_alternative<T>(config_values_.at(key));
  }

  /// @brief Retrieves a configuration entry of a specific type.
  /// @tparam T The value type to retrieve.
  /// @param key The configuration key.
  /// @return A pointer to the value if it exists with the correct type, nullptr
  /// otherwise.
  template <meta::one_of<bool, std::int64_t, double, std::string> T>
  const T* get(const key_type& key) const {
    return try_get<T>(key);
  }

  /// @brief Retrieves a configuration entry with a fallback value.
  /// @tparam T The value type to retrieve.
  /// @param key The configuration key.
  /// @param fallback The value to return if the entry doesn't exist or has
  /// wrong type.
  /// @return The configuration value or the fallback.
  template <meta::one_of<bool, std::int64_t, double, std::string> T>
  const T& get_or(const key_type& key, const T& fallback) const {
    if (auto ptr = try_get<T>(key)) {
      return *ptr;
    }
    return fallback;
  }

  /// @brief Retrieves all configuration entries.
  /// @return A const reference to the configuration dictionary.
  const dictionary& get_entries() const noexcept;

private:
  /// @brief Attempts to retrieve a typed configuration entry.
  /// Internal helper used by get() and get_or().
  /// @tparam T The value type to retrieve.
  /// @param key The configuration key.
  /// @return A const pointer to the value if it exists with correct type,
  /// nullptr otherwise.
  template <class T>
  const T* try_get(const key_type& key) const {
    if (!config_values_.contains(key)) {
      return nullptr;
    }
    const auto& value = config_values_.at(key);
    if (std::holds_alternative<T>(value)) {
      return std::addressof(std::get<T>(value));
    }
    return nullptr;
  }

  /// @brief The dictionary containing all configuration values.
  dictionary config_values_;
};

} // namespace util
