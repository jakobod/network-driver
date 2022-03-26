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

void binary_serializer::serialize(const std::string& str) {
  serialize(str.size());
  std::memcpy(free_space_.data(), str.data(), str.size());
}

void binary_serializer::realloc(std::size_t required_free_space) {
  if (free_space_.size() < required_free_space) {
    const auto missing = required_free_space - free_space_.size();
    buf_.resize(buf_.size() + missing);
    free_space_ = byte_span{buf_}.last(free_space_.size() + missing);
  }
}

} // namespace util
