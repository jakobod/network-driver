/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "fwd.hpp"

#include "openssl/tls_context.hpp"

#include "util/error.hpp"

#include <iostream>

[[noreturn]] void handle_error(const util::error& err) {
  std::cerr << "ERROR: " << err << std::endl;
  exit(-1);
}

void print_data_info(openssl::tls_session& client,
                     openssl::tls_session& server) {
  std::cout << "client has_more_data() = " << client.has_more_data()
            << ", size = " << client.write_buffer().size() << std::endl;
  std::cout << "server has_more_data() = " << server.has_more_data()
            << ", size = " << server.write_buffer().size() << std::endl;
}

void handle_handshake(openssl::tls_session& client,
                      openssl::tls_session& server) {
  for (size_t i = 0; i < 2; ++i) {
    if (auto err = server.consume(client.write_buffer()))
      handle_error(err);
    client.write_buffer().clear();

    if (auto err = client.consume(server.write_buffer()))
      handle_error(err);
    server.write_buffer().clear();
  }

  if (!client.is_initialized())
    handle_error(util::error(util::error_code::openssl_error,
                             "Client not initialized after handshakes!"));
  if (!server.is_initialized())
    handle_error(util::error(util::error_code::openssl_error,
                             "Server not initialized after handshakes!"));
}

int main(int, char**) {
  openssl::tls_context ctx;
  if (auto err = ctx.init(CERT_DIRECTORY "/server.crt",
                          CERT_DIRECTORY "/server.key"))
    handle_error(err);

  std::cout << "Context created and initialized" << std::endl;

  auto on_data = [](util::const_byte_span bytes) {
    std::cout << std::string{reinterpret_cast<const char*>(bytes.data()),
                             bytes.size()}
              << std::endl;
  };

  // get sessions and check for errors
  auto maybe_client_session = ctx.new_client_session(on_data);
  if (auto err = util::get_error(maybe_client_session))
    handle_error(*err);
  auto client_session = std::get<openssl::tls_session>(maybe_client_session);

  auto maybe_server_session = ctx.new_server_session(on_data);
  if (auto err = util::get_error(maybe_server_session))
    handle_error(*err);
  auto server_session = std::get<openssl::tls_session>(maybe_server_session);

  handle_handshake(client_session, server_session);
  std::cout << "Handshake done" << std::endl;

  std::string str{"Hello World"};

  std::cout << "enqueueing data" << std::endl;
  if (auto err = client_session.encrypt(
        {reinterpret_cast<const std::byte*>(str.data()), str.size()}))
    handle_error(err);

  print_data_info(client_session, server_session);

  std::cout << "Passing encrypted string: "
            << std::string(reinterpret_cast<const char*>(
                             client_session.write_buffer().data()),
                           client_session.write_buffer().size())
            << std::endl;

  util::byte_buffer buf;
  if (auto err = server_session.consume(client_session.write_buffer()))
    handle_error(err);
  client_session.write_buffer().clear();

  return 0;
}
