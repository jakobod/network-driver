/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "util/binary_serializer.hpp"

namespace util {

binary_serializer::binary_serializer(byte_buffer& buf)
  : buf_(buf), free_space_(buf_) {
  // nop
}

void binary_serializer::realloc(std::size_t required_free_space) {
  if (free_space_.size() < required_free_space) {
    const auto missing = required_free_space - free_space_.size();
    buf_.resize(buf_.size() + missing);
    free_space_ = byte_span{buf_}.last(required_free_space);
  }
}

void binary_serializer::serialize(const float& val) {
  union {
    float f;
    std::uint32_t i;
  };
  f = val;
  serialize(i);
}

void binary_serializer::serialize(const double& val) {
  union {
    double f;
    std::uint64_t i;
  };
  f = val;
  serialize(i);
}

} // namespace util
