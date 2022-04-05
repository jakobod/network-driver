/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <string>

namespace openssl {

enum class ssl_status {
  ok,
  want_io,
  fail,
};

std::string to_string(ssl_status status);

} // namespace openssl
