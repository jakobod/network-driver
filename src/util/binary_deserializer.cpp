/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "util/binary_deserializer.hpp"

namespace util {

binary_deserializer::binary_deserializer(const_byte_span bytes)
  : bytes_(bytes) {
  // nop
}

void binary_deserializer::deserialize(float& val) {
  union {
    float f;
    std::uint32_t i = 0;
  };
  deserialize(i);
  val = f;
}

void binary_deserializer::deserialize(double& val) {
  union {
    double f;
    std::uint64_t i = 0;
  };
  deserialize(i);
  val = f;
}

} // namespace util
