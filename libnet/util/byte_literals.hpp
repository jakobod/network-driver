/**
 *  @author    Jakob Otto
 *  @file      byte_literals.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <cstddef>

namespace util::byte_literals {

/// @brief User-defined literal for kilobytes.
/// Converts a numeric value to bytes (1 KB = 1024 bytes).
/// @param val The number of kilobytes.
/// @return The number of bytes as a std::size_t.
/// @example auto size = 10_KB;  // 10240 bytes
constexpr std::size_t operator""_KB(unsigned long long val) noexcept {
  return val * 1024ULL;
}

/// @brief User-defined literal for megabytes.
/// Converts a numeric value to bytes (1 MB = 1024 KB).
/// @param val The number of megabytes.
/// @return The number of bytes as a std::size_t.
/// @example auto size = 5_MB;  // 5242880 bytes
constexpr std::size_t operator""_MB(unsigned long long val) noexcept {
  return val * 1024ULL * 1024ULL;
}

/// @brief User-defined literal for gigabytes.
/// Converts a numeric value to bytes (1 GB = 1024 MB).
/// @param val The number of gigabytes.
/// @return The number of bytes as a std::size_t.
/// @example auto size = 2_GB;  // 2147483648 bytes
constexpr std::size_t operator""_GB(unsigned long long val) noexcept {
  return val * 1024ULL * 1024ULL * 1024ULL;
}

/// @brief User-defined literal for terabytes.
/// Converts a numeric value to bytes (1 TB = 1024 GB).
/// @param val The number of terabytes.
/// @return The number of bytes as a std::size_t.
/// @example auto size = 1_TB;  // 1099511627776 bytes
constexpr std::size_t operator""_TB(unsigned long long val) noexcept {
  return val * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
}

} // namespace util::byte_literals
