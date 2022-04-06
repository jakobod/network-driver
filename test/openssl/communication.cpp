/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "fwd.hpp"

#include <gtest/gtest.h>

#include "openssl/tls_context.hpp"
#include "openssl/tls_session.hpp"

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
  util::byte_buffer client_write_buffer;
  auto client = make_client_session(ctx, client_write_buffer, on_data);
  if (auto err = client.init())
    FAIL() << "Initializing client session failed: " << err;

  util::byte_buffer server_write_buffer;
  auto server = make_server_session(ctx, server_write_buffer, on_data);
  if (auto err = server.init())
    FAIL() << "Initializing server session failed: " << err;

  // Handle the TLS handshake
  handle_handshake(client, server);
  // Both sides should be initialized with completed handshake
  ASSERT_TRUE(client.is_initialized());
  ASSERT_TRUE(server.is_initialized());

  // Transmit data from the client to the server
  ASSERT_NO_ERROR(client.encrypt(util::make_const_byte_span(testString)));
  ASSERT_NO_ERROR(server.consume(client_write_buffer));
  client_write_buffer.clear();
  ASSERT_EQ(received_string, testString);
}
