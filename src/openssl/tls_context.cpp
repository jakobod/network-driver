/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "openssl/tls_context.hpp"

#include "util/error.hpp"

#include <openssl/err.h>

using util::error_code::openssl_error;

namespace openssl {

tls_context::tls_context() {
  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();
  ERR_load_BIO_strings();
  ERR_load_crypto_strings();
}

tls_context::~tls_context() {
  SSL_CTX_free(ctx_);
  ERR_free_strings();
}

util::error tls_context::init(const std::string& cert_path,
                              const std::string& key_path) {
  // create the SSL context
  ctx_ = SSL_CTX_new(SSLv23_method());
  if (!ctx_)
    return {openssl_error, "Failed to create SSL context"};

  // Load certificate file
  if (SSL_CTX_use_certificate_file(ctx_, cert_path.c_str(), SSL_FILETYPE_PEM)
      != 1)
    return {openssl_error, "SSL_CTX_use_certificate_file failed"};

  // Load key file
  if (SSL_CTX_use_PrivateKey_file(ctx_, key_path.c_str(), SSL_FILETYPE_PEM)
      != 1)
    return {openssl_error, "SSL_CTX_use_PrivateKey_file failed"};

  // Check certificate and key files
  if (SSL_CTX_check_private_key(ctx_) != 1)
    return {openssl_error, "SSL_CTX_check_private_key failed"};

  // Only allow TLSv1.2 and disable compression
  SSL_CTX_set_min_proto_version(ctx_, TLS1_3_VERSION);

  return util::none;
}

util::error_or<tls_session>
tls_context::new_client_session(tls_session::on_data_callback_type on_data) {
  tls_session session{ctx_, std::move(on_data), session_type::client};
  if (auto err = session.init())
    return err;
  return session;
}

util::error_or<tls_session>
tls_context::new_server_session(tls_session::on_data_callback_type on_data) {
  tls_session session{ctx_, std::move(on_data), session_type::server};
  if (auto err = session.init())
    return err;
  return session;
}

} // namespace openssl
