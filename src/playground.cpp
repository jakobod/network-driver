#include "net/operation.hpp"

#include <iostream>

int main(int, char**) {
  std::int8_t a = 0b11111111;
  std::int8_t b = 0b11111111;
  std::int16_t abu = a + b;

  // std::int16_t abs = a + b;
  std::cout << std::to_string(a) << " + " << std::to_string(b) << " = "
            << std::to_string(abu) << std::endl;
  // std::cout << std::to_string(abs) << std::endl;
  return 0;
}
