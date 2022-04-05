/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "fwd.hpp"

#include <gtest/gtest.h>

#include "openssl/tls_context.hpp"

#include <string>

using namespace openssl;

using namespace std::string_literals;

namespace {

const std::string testString = "Hello World";

#define ASSERT_NO_ERROR(expr)                                                  \
  if (auto err = expr)                                                         \
    FAIL() << #expr << " returned an error: " << err << std::endl;

void handle_handshake(tls_session& client, tls_session& server) {
  // It should take exactly two roundtrips for the handshake to complete
  for (size_t i = 0; i < 2; ++i) {
    // "Send" client data
    ASSERT_NO_ERROR(server.consume(client.write_buffer()));
    client.write_buffer().clear();

    // "Send" server data
    ASSERT_NO_ERROR(client.consume(server.write_buffer()));
    server.write_buffer().clear();
  }
}

} // namespace

TEST(openssl_test, roundtrip) {
  tls_context ctx;
  ASSERT_NO_ERROR(
    ctx.init(CERT_DIRECTORY "/server.crt", CERT_DIRECTORY "/server.key"));

  std::string received_string;
  auto on_data = [&received_string](util::const_byte_span bytes) {
    received_string = std::string{reinterpret_cast<const char*>(bytes.data()),
                                  bytes.size()};
  };

  // get sessions and check for errors
  auto maybe_client_session = ctx.new_client_session(on_data);
  if (auto err = util::get_error(maybe_client_session))
    FAIL() << "Creating client session failed: " << *err;
  auto client_session = std::get<tls_session>(maybe_client_session);

  auto maybe_server_session = ctx.new_server_session(on_data);
  if (auto err = util::get_error(maybe_server_session))
    FAIL() << "Creating server session failed: " << *err;
  auto server_session = std::get<tls_session>(maybe_server_session);

  // Handle the TLS handshake
  handle_handshake(client_session, server_session);
  // Both sides should be initialized with completed handshake
  ASSERT_TRUE(client_session.is_initialized());
  ASSERT_TRUE(server_session.is_initialized());

  // Transmit data from the client to the server
  ASSERT_NO_ERROR(
    client_session.encrypt(util::make_const_byte_span(testString)));
  ASSERT_NO_ERROR(server_session.consume(client_session.write_buffer()));
  client_session.write_buffer().clear();
  ASSERT_EQ(received_string, testString);
}
