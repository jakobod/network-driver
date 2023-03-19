#include "util/cli_parser.hpp"

#include <iostream>

using namespace std::chrono_literals;
using namespace std::string_literals;

namespace {

constexpr const std::string_view id_no_opt = "no-opt";
constexpr const std::string_view id_yes_opt = "yes-opt";

} // namespace

int main(int argc, char** argv) {
  util::cli_parser parser;
  try {
    parser.add_option(id_yes_opt.data(), "no-opt", 'n', false)
      .add_option(id_yes_opt.data(), "yes-opt", 'y', true);
  } catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    return EXIT_FAILURE;
  }

  try {
    parser.parse(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Number of values for " << id_yes_opt << ": "
            << parser.num_option_values(id_yes_opt.data()) << std::endl;
  for (const auto& v : parser.option_values(id_yes_opt.data()))
    std::cout << v << std::endl;
}
