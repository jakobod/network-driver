/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <functional>
#include <openssl/ssl.h>

namespace openssl {

struct client_session {
public:
  client_session(SSL_CTX* ctx)
    : rbio{BIO_new(BIO_s_mem())},
      wbio{BIO_new(BIO_s_mem())},
      ssl{SSL_new(ctx)} {
    // Sets ssl to work in server mode.
    SSL_set_accept_state(ssl);
    SSL_set_bio(ssl, rbio, wbio);
  }

  ~client_session() {
    SSL_free(ssl); /* free the SSL object and its BIO's */
    free(write_buf);
    free(encrypt_buf);
  }

  bool has_more_data() {
    return (write_len > 0);
  }

private:
  /// Don't know currently
  SSL* ssl;

  /// Write access to the SSL
  BIO* rbio;
  /// Read access from the SSL
  BIO* wbio;

  /* Bytes waiting to be written to socket. This is data that has been generated
   * by the SSL object, either due to encryption of user input, or, writes
   * requires due to peer-requested SSL renegotiation. */

  char* write_buf;
  size_t write_len;

  /* Bytes waiting to be fed into the SSL object for encryption. */
  char* encrypt_buf;
  size_t encrypt_len;

  /* Method to invoke when unencrypted bytes are available. */
  std::function<void(char* buf, size_t len)> func_;
};

} // namespace openssl
