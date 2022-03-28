/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "fwd.hpp"

#include "meta/type_traits.hpp"

#include "util/binary_serializer.hpp"

#include <iostream>

using namespace util;

int main(int, char**) {
  byte_buffer buf;
  binary_serializer serializer{buf};
  serializer(float{19.0}, double{20.0});
  for (const auto& b : buf)
    std::cout << std::hex << "0x" << static_cast<std::uint16_t>(b) << ", ";
  std::cout << std::endl;
  return 0;
}
