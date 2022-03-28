/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "fwd.hpp"

#include "util/binary_serializer.hpp"

#include <iostream>

struct dummy {
  void operator()() {
    std::cout << "THIS WORKS" << std::endl;
  }
};

template <class T, std::enable_if_t<meta::is_container_v<T>>* = nullptr>
void visit(T& t) {
  std::cout << "is container" << std::endl;
}

// template <class T, std::enable_if_t<std::is_integral_v<T>>* = nullptr>
// void visit(T& t) {
//   std::cout << "is integral" << std::endl;
// }

template <class T, std::enable_if_t<std::is_integral_v<T>>* = nullptr>
void check() {
  std::cout << "is_integral" << std::endl;
}

template <class T, std::enable_if_t<meta::is_container_v<T>>* = nullptr>
void check() {
  std::cout << "is_container" << std::endl;
}

int main(int, char**) {
  check<std::uint8_t>();
  check<std::vector<uint8_t>>();

  return 0;
}
