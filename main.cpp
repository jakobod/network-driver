/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "openssl/server_session.hpp"
#include "openssl/tls_context.hpp"

#include <iostream>

int main(int, char**) {
  openssl::tls_context ctx;
  if (auto err = ctx.init(CERT_DIRECTORY "/server.crt",
                          CERT_DIRECTORY "/server.key"))
    std::cerr << "[Failed to init session] " << err << std::endl;
  else
    std::cerr << "Created tls_context" << std::endl;

  auto session = ctx.new_client();

  return 0;
}
