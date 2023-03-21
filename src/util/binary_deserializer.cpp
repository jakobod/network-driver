/**
 *  @author    Jakob Otto
 *  @file      binary_deserializer.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
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
