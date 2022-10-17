/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <cstddef>
#include <span>
#include <variant>

namespace util {

// -- classes ------------------------------------------------------------------

class error;
class binary_deserializer;
class binary_serializer;
class serialized_size;

// -- type aliases -------------------------------------------------------------

using byte_span = std::span<std::byte>;
using const_byte_span = std::span<const std::byte>;

// -- template types -----------------------------------------------------------

template <class T>
using error_or = std::variant<T, error>;
template <class Func>
class scope_guard;

} // namespace util
