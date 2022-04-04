#include "util/error.hpp"

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

namespace {

class tls_session {
public:
  tls_session() {
    /* SSL library initialisation */
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
  }

  error init() {
    /* create the SSL server context */
    auto ctx = SSL_CTX_new(SSLv23_server_method());
    if (!ctx)
      die("SSL_CTX_new()");

    /* Load certificate and private key files, and check consistency  */
    int err;
    err = SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM);
    if (err != 1)
      int_error("SSL_CTX_use_certificate_file failed");
    else
      printf("certificate file loaded ok\n");

    /* Indicate the key file to be used */
    err = SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM);
    if (err != 1)
      int_error("SSL_CTX_use_PrivateKey_file failed");
    else
      printf("private-key file loaded ok\n");

    /* Make sure the key and certificate file match. */
    if (SSL_CTX_check_private_key(ctx) != 1)
      int_error("SSL_CTX_check_private_key failed");
    else
      printf("private key verified ok\n");

    /* Recommended to avoid SSLv2 & SSLv3 */
    SSL_CTX_set_options(ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
  }

private:
};

void ssl_init() {
}

} // namespace
