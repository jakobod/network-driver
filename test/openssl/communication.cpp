/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "fwd.hpp"

#include "openssl/tls_context.hpp"
#include "openssl/tls_session.hpp"

#include "net_test.hpp"

#include <string>

using namespace openssl;

using namespace std::string_literals;

namespace {

const std::string testString = "Hello World";

void handle_handshake(tls_session& client, tls_session& server) {
  // It should take exactly two roundtrips for the handshake to complete
  for (size_t i = 0; i < 2; ++i) {
    // "Send" client data
    EXPECT_NO_ERROR(server.consume(client.write_buffer()));
    client.write_buffer().clear();

    // "Send" server data
    EXPECT_NO_ERROR(client.consume(server.write_buffer()));
    server.write_buffer().clear();
  }
}

} // namespace

TEST(openssl_test, roundtrip) {
  tls_context ctx;
  EXPECT_NO_ERROR(
    ctx.init(CERT_DIRECTORY "/server.crt", CERT_DIRECTORY "/server.key"));

  std::string received_string;
  auto on_data = [&received_string](util::const_byte_span bytes) {
    received_string = std::string{reinterpret_cast<const char*>(bytes.data()),
                                  bytes.size()};
  };

  // get sessions and check for errors
  util::byte_buffer client_write_buffer;
  auto client = make_client_session(ctx, client_write_buffer, on_data);
  EXPECT_NO_ERROR(client.init());

  util::byte_buffer server_write_buffer;
  auto server = make_server_session(ctx, server_write_buffer, on_data);
  EXPECT_NO_ERROR(server.init());

  // Handle the TLS handshake
  handle_handshake(client, server);
  // Both sides should be initialized with completed handshake
  ASSERT_TRUE(client.is_initialized());
  ASSERT_TRUE(server.is_initialized());

  // Transmit data from the client to the server
  EXPECT_NO_ERROR(client.encrypt(util::make_const_byte_span(testString)));
  EXPECT_NO_ERROR(server.consume(client_write_buffer));
  client_write_buffer.clear();
  ASSERT_EQ(received_string, testString);
}
