/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <variant>

namespace util {

class error;
class binary_deserializer;
class binary_serializer;
class serialized_size;

template <class T>
using error_or = std::variant<T, error>;
template <class Func>
class scope_guard;

} // namespace util
