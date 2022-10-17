/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <variant>

namespace util {

// -- classes ------------------------------------------------------------------

class binary_deserializer;
class binary_serializer;
class error;
class serialized_size;

// -- enums --------------------------------------------------------------------

enum class error_code : std::uint8_t;

// -- type aliases -------------------------------------------------------------

using byte_span = std::span<std::byte>;
using const_byte_span = std::span<const std::byte>;

// -- template types -----------------------------------------------------------

template <size_t Size>
using byte_array = std::array<std::byte, Size>;
template <class T>
using error_or = std::variant<T, error>;
template <class Func>
class scope_guard;

} // namespace util
