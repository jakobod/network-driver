#include "net_test.hpp"
#include "util/logger.hpp"

int main(int argc, char** argv) {
  // Disables logging alltogether
  LOG_INIT(false, "");
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
