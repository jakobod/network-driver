/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "meta/concepts.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>

namespace util {

/// @brief Configuration class for specifying config params using a config-file
class config {
public:
  // -- Public member types ----------------------------------------------------

  /// @brief Type of the key used for mapping the entries
  using key_type = std::string;
  /// @brief Type of the entries
  using entry_type = std::variant<bool, std::int64_t, double, std::string>;
  /// @brief Type of the dictionary used for mapping the entries
  using dictionary = std::unordered_map<key_type, entry_type>;

  /// @brief Constructs an empty config instance
  config() = default;

  /// @brief Constructs a config instance from a config file
  /// @param config_path  Path to the config file
  config(const std::string& config_path);

  /// @brief Copy constructs a config instance from another instance
  /// @param other  The instance to copy
  config(const config& other) = default;

  /// @brief Move constructs a config instance from another instance
  /// @param other  The instance to move
  config(config&& other) = default;

  /// @brief Copy assigns a config instance from another instance
  /// @param other  The instance to copy
  config& operator=(const config& other) noexcept = default;

  /// @brief Move assigns a config instance from another instance
  /// @param other  The instance to copy
  config& operator=(config&& other) noexcept = default;

  /// @brief Parses a given config file and adds the values to this instance
  /// @param config_path  Path to the config file
  void parse(const std::string& config_path);

  /// @brief Adds a single entry to the config
  /// @tparam Entry  Type of the entry
  /// @param key  The key to add the entry at
  /// @param entry  The entry to add
  template <meta::one_of<bool, std::int64_t, double, std::string> Entry>
  void add_config_entry(key_type key, Entry entry) {
    config_values_.emplace(std::move(key), std::move(entry));
  }

  /// @brief Checks wether the config contains an entry for `key` and the type
  ///        is equal to `T`
  /// @tparam T  The expected type of the entry
  /// @param key  Key of the entry
  /// @returns true in case the config contains an entry for `key` and the type
  ///          is equal to `T`, false otherwise
  template <meta::one_of<bool, std::int64_t, double, std::string> T>
  bool has_entry(const key_type& key) const {
    if (!config_values_.contains(key))
      return false;
    return std::holds_alternative<T>(config_values_.at(key));
  }

  /// @brief Retrieves a config entry from the config
  /// @tparam T  type of the config entry
  /// @param key  The key of the config entry
  /// @returns A pointer to the config entry if it exists, nullptr when the
  ///          entry does not have type T or does not exist
  template <meta::one_of<bool, std::int64_t, double, std::string> T>
  const T* get(const key_type& key) const {
    return try_get<T>(key);
  }

  /// @brief Retrieves a config entry from the config
  /// @tparam T  type of the config entry
  /// @param key  The key of the config entry
  /// @param fallback  The fallback value in case of errors
  /// @returns The entry value if it exists and has type `T`, the fallback
  ///          otherwise
  template <meta::one_of<bool, std::int64_t, double, std::string> T>
  const T& get_or(const key_type& key, const T& fallback) const {
    if (auto ptr = try_get<T>(key))
      return *ptr;
    return fallback;
  }

  const dictionary& get_entries() const noexcept;

private:
  /// @brief Tries to retrieve a config entry from the config
  /// @tparam T  type of the config entry
  /// @param key  The key of the config entry
  /// @returns A pointer to the config entry if it exists, nullptr in case the
  ///          entry does not have type T or does not exist
  template <class T>
  const T* try_get(const key_type& key) const {
    if (!config_values_.contains(key))
      return nullptr;
    const auto& value = config_values_.at(key);
    if (std::holds_alternative<T>(value))
      return std::addressof(std::get<T>(value));
    return nullptr;
  }

  /// @brief The dictionary containing the config_values
  dictionary config_values_;
};

} // namespace util
