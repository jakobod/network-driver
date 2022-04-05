/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "openssl/server_session.hpp"

#include "util/error.hpp"

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

namespace openssl {

class tls_context {
public:
  tls_context() {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
  }

  ~tls_context() {
    SSL_CTX_free(ctx_);
  }

  util::error init(const std::string& cert_path, const std::string& key_path) {
    /* create the SSL server context */
    ctx_ = SSL_CTX_new(SSLv23_server_method());
    if (!ctx_)
      return util::error(util::error_code::openssl_error,
                         "Failed to create SSL context");

    /* Load certificate and private key files, and check consistency  */
    if (SSL_CTX_use_certificate_file(ctx_, cert_path.c_str(), SSL_FILETYPE_PEM)
        != 1)
      return util::error(util::error_code::openssl_error,
                         "SSL_CTX_use_certificate_file failed");

    /* Indicate the key file to be used */
    if (SSL_CTX_use_PrivateKey_file(ctx_, key_path.c_str(), SSL_FILETYPE_PEM)
        != 1)
      return util::error(util::error_code::openssl_error,
                         "SSL_CTX_use_PrivateKey_file failed");

    /* Make sure the key and certificate file match. */
    if (SSL_CTX_check_private_key(ctx_) != 1)
      return util::error(util::error_code::openssl_error,
                         "SSL_CTX_check_private_key failed");

    /* Recommended to avoid SSLv2 & SSLv3 */
    SSL_CTX_set_options(ctx_, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3
                                | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
    return util::none;
  }

  server_session new_client() {
    return {ctx_};
  }

private:
  SSL_CTX* ctx_;
};

} // namespace openssl
