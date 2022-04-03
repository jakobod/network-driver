/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "fwd.hpp"

#include "util/format.hpp"

#include <iostream>
#include <regex>

using namespace util;
using namespace std::string_literals;

struct dummy {
  std::vector<std::uint8_t> buf;
};

int main(int, char**) {
  std::string form = "Hello {0}";
  auto res = util::format("Hello {0}", "world");
  std::cout << form << " <-> " << res << std::endl;
  // byte_buffer buf;
  // binary_serializer serializer{buf};
  // serializer(std::make_pair(std::uint32_t{420}, std::uint64_t{69}));
  // for (const auto& b : buf)
  //   std::cout << std::hex << "0x" << static_cast<std::uint16_t>(b) << ", ";
  // std::cout << std::endl;
  return 0;
}
