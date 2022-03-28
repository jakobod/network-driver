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

int main(int, char**) {
  std::cout << "has data member = "
            << meta::has_data_member_v<std::vector<std::uint8_t>> << std::endl;
  std::cout << "has size member = "
            << meta::has_size_member_v<std::vector<std::uint8_t>> << std::endl;
}
