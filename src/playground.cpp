#include "util/logger.hpp"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

int main(int, char**) {
  if (auto err = util::logger::init(util::logger::config{false, "test.log"}))
    std::terminate();
  LOG_DEBUG("debug");
  LOG_INFO("info");
  LOG_WARNING("warning");
  LOG_ERROR("error");
  return 0;
}
