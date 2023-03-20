#include "meta/concepts.hpp"
#include "util/config.hpp"

#include <iostream>

using namespace std::string_literals;

int main(int argc, const char** argv) {
  util::config cfg;
  // cfg.add_config_entry("logger.file-name", "/var/log/network-driver.log"s);
  // cfg.add_config_entry("logger.log-to-terminal", true);

  const auto file_name = cfg.get_or<std::string>("logger.file-name",
                                                 "logfile.log");
  std::cout << "size = " << file_name.size() << std::endl;
  std::cout << file_name << std::endl;

  std::cout << (cfg.get_or<bool>("logger.log-to-terminal", false) ? "true"
                                                                  : "false")
            << std::endl;
  // struct dummy {};
  // [[maybe_unused]] auto d = cfg.get<dummy>("logger.log-to-terminal");
}
