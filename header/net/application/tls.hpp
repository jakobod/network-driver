/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include "util/error.hpp"

#include <openssl/err.h>
#include <openssl/ssl.h>

namespace {

using ssl_ctx_ptr = std::unique_ptr<SSL_CTX, void (*)(SSL_CTX*)>;

std::string openssl_error_as_string() {
  BIO* bio = BIO_new(BIO_s_mem());
  ERR_print_errors(bio);
  char* buf;
  size_t len = BIO_get_mem_data(bio, &buf);
  std::string ret(buf, len);
  BIO_free(bio);
  return ret;
}

error_ot ssl_ctx_ptr create_server_context() {
  auto ctx = ssl_ctx_ptr(SSL_CTX_new(TLS_server_method()), SSL_CTX_free);
  if (!ctx)
    return net::error{net::error_code::openssl_error,
                      "Unable to create SSL context"};
  // ERR_print_errors_fp(stderr);
  return ctx;
}

} // namespace

namespace application {

template <class Application>
class tls {
  template <class... Ts>
  tls(bool is_client, Ts&&... xs)
    : application_(std::forward<Ts>(xs)...),
      is_client_{is_client},
      ssl_context_(create_server_context()) {
    // nop
  }

  template <class Parent>
  error init(Parent& parent) {
  }

  template <class Parent>
  event_result produce(Parent& parent) {
    return event_result::ok;
  }

  bool has_more_data() {
    return false;
  }

  template <class Parent>
  event_result consume(Parent& parent, util::const_byte_span data) {
    return event_result::ok;
  }

  template <class Parent>
  event_result handle_timeout(Parent&, uint64_t) {
    return event_result::ok;
  }

private:
  Application application_;

  bool is_client_;

  // SSL Context
  std::unique_ptr<SSL_CTX> ssl_context_;
};

} // namespace application
