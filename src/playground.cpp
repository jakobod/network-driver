#include "util/logger.hpp"
#include "util/scope_guard.hpp"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace beebop {
struct something {
  static void do_something() {
    LOG_TRACE();
    LOG_DEBUG("Debug");
    LOG_WARNING("Warning");
    LOG_ERROR("Error");
  }
};
} // namespace beebop

int main(int, char**) {
  LOG_INIT(true, "");
  LOG_TRACE();
  beebop::something::do_something();
  return 0;
}
