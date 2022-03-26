/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include "util/serialized_size.hpp"

#include "meta/type_traits.hpp"

#include <cstring>
#include <numeric>
#include <type_traits>

namespace util {

class binary_serializer {
public:
  explicit binary_serializer(byte_buffer& buf);

  template <class T, class... Ts>
  void operator()(const T& what, const Ts&... ts) {
    realloc(serialized_size::calculate(what, ts...));
    unpack(what, ts...);
  }

private:
  template <class T, class... Ts>
  void unpack(const T& what, const Ts&... ts) {
    serialize(what);
    unpack(ts...);
  }

  template <class T>
  void unpack(const T& what) {
    serialize(what);
  }

  template <class T, std::enable_if_t<meta::is_visitable_v<T>>* = nullptr>
  void serialize([[maybe_unused]] const T& what) {
    visit(what, *this);
  }

  template <class T,
            std::enable_if_t<std::numeric_limits<T>::is_integer>* = nullptr>
  void serialize(T i) {
    std::memcpy(free_space_.data(), &i, sizeof(T));
  }

  template <class C, std::enable_if_t<meta::is_container_v<C>>* = nullptr>
  void serialize([[maybe_unused]] const C& container) {
    serialize(container.size());
    for (const auto& val : container)
      serialize(val);
  }

  void serialize(const std::string& str);

  void realloc(std::size_t required_free_space);

  byte_buffer& buf_;
  byte_span free_space_;
};

} // namespace util
