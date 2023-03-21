/**
 *  @author    Jakob Otto
 *  @file      tls_context.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "openssl/tls_context.hpp"

#include "util/error.hpp"
#include "util/error_code.hpp"

#include <openssl/err.h>

namespace openssl {

tls_context::tls_context() {
  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();
#if OPENSSL_VERSION_NUMBER >= 0x30000000L && !defined(LIBRESSL_VERSION_NUMBER)
  // OpenSSL now loads error strings automatically so these functions are not
  // needed.
#else
  /* Load error strings into mem*/
  ERR_load_BIO_strings();
  ERR_load_crypto_strings();
#endif
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
    return {util::error_code::openssl_error, "Failed to create SSL context"};

  // Load certificate file
  if (SSL_CTX_use_certificate_file(ctx_, cert_path.c_str(), SSL_FILETYPE_PEM)
      != 1)
    return {util::error_code::openssl_error,
            "SSL_CTX_use_certificate_file failed"};

  // Load key file
  if (SSL_CTX_use_PrivateKey_file(ctx_, key_path.c_str(), SSL_FILETYPE_PEM)
      != 1)
    return {util::error_code::openssl_error,
            "SSL_CTX_use_PrivateKey_file failed"};

  // Check certificate and key files
  if (SSL_CTX_check_private_key(ctx_) != 1)
    return {util::error_code::openssl_error,
            "SSL_CTX_check_private_key failed"};

  // Only allow TLSv1.2 and disable compression
  SSL_CTX_set_min_proto_version(ctx_, TLS1_3_VERSION);

  return util::none;
}

} // namespace openssl
