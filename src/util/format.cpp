/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "util/format.hpp"

namespace util {

std::vector<std::string> split(std::string str, const std::string delim) {
  std::vector<std::string> tokens;
  size_t pos = 0;
  while ((pos = str.find(delim)) != std::string::npos) {
    tokens.emplace_back(str.substr(0, pos));
    str.erase(0, pos + delim.length());
  }
  tokens.emplace_back(std::move(str));
  return tokens;
}

std::vector<std::string> split(std::string str, const char delim) {
  std::vector<std::string> tokens;
  std::stringstream stream(str);
  std::string token;
  while (std::getline(stream, token, delim))
    tokens.emplace_back(std::move(token));
  return tokens;
}

} // namespace util
