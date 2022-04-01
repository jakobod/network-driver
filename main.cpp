/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "fwd.hpp"

#include "meta/type_traits.hpp"

#include "util/binary_serializer.hpp"

#include <iostream>

using namespace util;

struct dummy {
  std::vector<std::uint8_t> buf;
};

int main(int, char**) {
  std::cout << "std::string = "
            << meta::has_resize_member_v<std::string> << std::endl;
  // byte_buffer buf;
  // binary_serializer serializer{buf};
  // serializer(std::make_pair(std::uint32_t{420}, std::uint64_t{69}));
  // for (const auto& b : buf)
  //   std::cout << std::hex << "0x" << static_cast<std::uint16_t>(b) << ", ";
  // std::cout << std::endl;
  return 0;
}
