#include <util/logger.hpp>

int main(int, char**) {
  LOG_DEBUG("debug");
  LOG_INFO("info");
  LOG_WARNING("warning");
  LOG_ERROR("error");
  return 0;
}
