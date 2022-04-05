/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include "util/error.hpp"

#include "openssl/ssl_status.hpp"

#include <openssl/ssl.h>

#include <functional>
#include <iostream>

namespace openssl {

struct server_mode {};
struct client_mode {};

/// TLS-session. Abstracts the en/decrypting of data
struct tls_session {
public:
  tls_session(SSL_CTX* ctx, server_mode);

  tls_session(SSL_CTX* ctx, client_mode);

  ~tls_session();

  bool has_more_data();

  bool is_initialized() {
    return SSL_is_init_finished(ssl_);
  }

  /* Queue encrypted bytes for socket write. Should only be used when the SSL
   * object has requested a write operation. */
  void queue_encrypted_bytes(util::const_byte_span bytes);

  void queue_plain_bytes(util::const_byte_span bytes);

  /// Takes in encrypted bytes, decrypts them, and passes it to a callback
  util::error consume_encrypted(util::const_byte_span bytes);

  util::error handle_handshake();

  /* Process outbound unencrypted data that are waiting to be encrypted.  The
   * waiting data resides in encrypt_buf.  It needs to be passed into the SSL
   * object for encryption, which in turn generates the encrypted bytes that
   * then will be queued for later socket write. */
  util::error produce(util::const_byte_span bytes);

  util::byte_buffer& write_buffer() {
    return write_buf_;
  }

  util::byte_buffer& encrypt_buffer() {
    return encrypt_buf_;
  }

private:
  void on_data(util::const_byte_span bytes) {
    std::cout << "[" << me << "] received: "
              << std::string(reinterpret_cast<const char*>(bytes.data()),
                             bytes.size())
              << std::endl;
  }

  ssl_status get_ssl_status(int n);

  const std::string me;

  /// SSL state
  SSL* ssl_;
  /// Write access to the SSL
  BIO* rbio_;
  /// Read access from the SSL
  BIO* wbio_;

  /* Bytes waiting to be written to socket. This is data that has been generated
   * by the SSL object, either due to encryption of user input, or, writes
   * requires due to peer-requested SSL renegotiation. */
  util::byte_buffer write_buf_;
  /* Bytes waiting to be fed into the SSL object for encryption. */
  util::byte_buffer encrypt_buf_;
};

} // namespace openssl
