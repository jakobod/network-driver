/**
 *  @author    Jakob Otto
 *  @file      binary_serializer.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "util/serialized_size.hpp"

#include "meta/concepts.hpp"

#include "util/byte_buffer.hpp"
#include "util/byte_span.hpp"

#include <cstring>
#include <tuple>
#include <utility>

namespace util {

class binary_serializer {
public:
  explicit binary_serializer(util::byte_buffer& buf);

  template <class... Ts>
  void operator()(const Ts&... ts) {
    realloc(serialized_size{}(ts...));
    (serialize(ts), ...);
  }

private:
  void realloc(std::size_t required_free_space);

  template <meta::trivially_serializable T>
  void serialize(const T& i) {
    std::memcpy(free_space_.data(), &i, sizeof(T));
    free_space_ = free_space_.subspan(sizeof(T));
  }

  void serialize(const float& val);

  void serialize(const double& val);

  template <class T, class U>
  void serialize(const std::pair<T, U>& p) {
    serialize(p.first);
    serialize(p.second);
  }

  template <class... Ts>
  void serialize(const std::tuple<Ts...>& t) {
    (serialize(std::get<Ts>(t)), ...);
  }

  template <meta::visitable T>
  void serialize(const T& what) {
    const_cast<T&>(what).visit(*this);
  }

  template <class T, size_t Size>
  void serialize(const T (&arr)[Size]) noexcept {
    serialize(arr, Size);
  }

  template <meta::container T>
  void serialize(const T& container) {
    serialize(container.data(), container.size());
  }

  // -- range serialize functions ----------------------------------------------

  // Serializes integral types
  template <meta::trivially_serializable T>
  void serialize(const T* ptr, std::size_t size) {
    serialize(size);
    const auto num_bytes = size * sizeof(T);
    std::memcpy(free_space_.data(), ptr, num_bytes);
    free_space_ = free_space_.subspan(num_bytes);
  }

  // Serializes visitable types
  template <meta::not_trivially_serializable T>
  void serialize(const T* ptr, std::size_t size) {
    serialize(size);
    for (const auto& val : make_span(ptr, size)) serialize(val);
  }

  byte_buffer& buf_;
  byte_span free_space_;
};

} // namespace util
