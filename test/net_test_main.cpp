#include "net_test.hpp"
#include "util/logger.hpp"

int main(int argc, char** argv) {
  // Disables logging alltogether
  if (auto err = util::logger::init({false, ""})) {
    MESSAGE() << "Could not configure logging" << std::endl;
    std::terminate();
  }
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
