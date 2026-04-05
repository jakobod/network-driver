/**
 *  @author    Jakob Otto
 *  @file      fwd.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <variant>

namespace util {

// -- classes ------------------------------------------------------------------

/// @brief Forward declaration of binary deserializer class.
class binary_deserializer;

/// @brief Forward declaration of binary serializer class.
class binary_serializer;

/// @brief Forward declaration of command-line parser class.
class cli_parser;

/// @brief Forward declaration of configuration storage class.
class config;

/// @brief Forward declaration of error class.
class error;

/// @brief Forward declaration of serialized size calculator class.
class serialized_size;

// -- enums ----------------------------------------------------------

/// @brief Forward declaration of error code enumeration.
enum class error_code : std::uint8_t;

// -- type aliases ---------------------------------------------------

/// @brief Type alias for mutable byte spans.
using byte_span = std::span<std::byte>;

/// @brief Type alias for const byte spans.
using const_byte_span = std::span<const std::byte>;

// -- template types --------------------------------------------------

/// @brief Fixed-size byte array type alias.
/// @tparam Size The size of the array in bytes.
template <size_t Size>
using byte_array = std::array<std::byte, Size>;

/// @brief Variant type for error handling (value or error).
/// @tparam T The type of the successful value.
template <class T>
using error_or = std::variant<T, error>;

/// @brief RAII scope guard for callable cleanup actions.
/// @tparam Func The type of the cleanup callable.
template <class Func>
class scope_guard;

} // namespace util
