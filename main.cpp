/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "fwd.hpp"

#include "util/binary_serializer.hpp"

#include <iostream>

int main(int, char**) {
  static constexpr const auto expected_result = util::make_byte_array(0x2A);

  util::byte_buffer buf;
  util::binary_serializer serializer{buf};
  serializer(std::uint8_t{42});
}
