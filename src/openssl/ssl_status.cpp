/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "openssl/ssl_status.hpp"

#include <string>

namespace openssl {

std::string to_string(ssl_status status) {
  switch (status) {
    case ssl_status::ok:
      return "ok";
    case ssl_status::want_io:
      return "want_io";
    case ssl_status::fail:
      return "fail";
    default:
      return "???";
  }
}

} // namespace openssl
