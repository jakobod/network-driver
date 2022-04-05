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
}

util::error tls_context::init(const std::string& cert_path,
                              const std::string& key_path) {
  /* create the SSL server context */
  ctx_ = SSL_CTX_new(SSLv23_method());
  if (!ctx_)
    return {openssl_error, "Failed to create SSL context"};

  /* Load certificate and private key files, and check consistency  */
  if (SSL_CTX_use_certificate_file(ctx_, cert_path.c_str(), SSL_FILETYPE_PEM)
      != 1)
    return {openssl_error, "SSL_CTX_use_certificate_file failed"};

  /* Indicate the key file to be used */
  if (SSL_CTX_use_PrivateKey_file(ctx_, key_path.c_str(), SSL_FILETYPE_PEM)
      != 1)
    return {openssl_error, "SSL_CTX_use_PrivateKey_file failed"};

  /* Make sure the key and certificate file match. */
  if (SSL_CTX_check_private_key(ctx_) != 1)
    return {openssl_error, "SSL_CTX_check_private_key failed"};

  /* Recommended to avoid SSLv2 & SSLv3 */
  SSL_CTX_set_options(ctx_, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

  return util::none;
}

util::error_or<tls_session> tls_context::new_client_session() {
  tls_session session{ctx_, client_mode{}};
  if (auto err = session.handle_handshake())
    return err;
  return session;
}

util::error_or<tls_session> tls_context::new_server_session() {
  return tls_session{ctx_, server_mode{}};
}

} // namespace openssl
