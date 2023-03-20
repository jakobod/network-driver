/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "meta/concepts.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>

namespace util {

class config {
public:
  // -- Public member types ----------------------------------------------------

  using key_type = std::string;

private:
  // -- Private member types ---------------------------------------------------

  using value_type = std::variant<bool, std::int64_t, double, std::string>;
  using dictionary = std::unordered_map<key_type, value_type>;

public:
  config() = default;

  config(const std::string& config_path_);

  template <meta::one_of<bool, std::int64_t, double, std::string> Entry>
  void add_config_entry(key_type key, Entry entry) {
    config_values_.emplace(std::move(key), std::move(entry));
  }

  template <meta::one_of<bool, std::int64_t, double, std::string> T>
  const T* get(const key_type& key) const {
    return try_get<T>(key);
  }

  template <meta::one_of<bool, std::int64_t, double, std::string> T>
  const T& get_or(const key_type& key, const T& fallback) const {
    if (auto ptr = try_get<T>(key))
      return *ptr;
    return fallback;
  }

private:
  template <class T>
  const T* try_get(const key_type& key) const {
    if (!config_values_.contains(key))
      return nullptr;
    const auto& value = config_values_.at(key);
    if (std::holds_alternative<T>(value))
      return std::addressof(std::get<T>(value));
    return nullptr;
  }

  dictionary config_values_;
};

} // namespace util
