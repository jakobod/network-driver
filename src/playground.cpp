#include "util/logger.hpp"
#include "util/scope_guard.hpp"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

int main(int, char**) {
  LOG_TRACE();
  LOG_DEBUG("DEBUG");
  LOG_WARNING("WARNING");
  LOG_ERROR("ERROR");
  return 0;
}
