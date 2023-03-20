#include "meta/concepts.hpp"

#include "util/config.hpp"
#include "util/logger.hpp"

#include <iostream>

using namespace std::string_literals;

int main(int argc, const char** argv) {
  util::config cfg;
  try {
    cfg.parse("config.cfg");
  } catch (const std::runtime_error& err) {
    LOG_ERROR(err.what());
  }

  std::cout << cfg.get_or<std::string>("logger.file-name", "logfile.log")
            << std::endl;

  std::cout << cfg.get_or<std::string>("logger.terminal-logging", "blubb")
            << std::endl;
}
