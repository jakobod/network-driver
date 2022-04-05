/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include "util/error.hpp"

#include <openssl/ssl.h>

#include <functional>
#include <iostream>

namespace openssl {

enum class session_type {
  server,
  client,
};

/// TLS-session. Abstracts the en/decrypting of data
class tls_session {
public:
  /// Callback type for on_data callback
  using on_data_callback_type = std::function<void(util::const_byte_span)>;

  // -- Constructors/Destructors/initialization --------------------------------

  /// Constructs a TLS server session
  tls_session(SSL_CTX* ctx, on_data_callback_type on_data, session_type type);

  /// Destructs a TLS session
  ~tls_session();

  /// Initializes a TLS session object
  util::error init();

  // -- member access ----------------------------------------------------------

  /// Checks wether data is left for writing
  bool has_more_data() {
    return !write_buf_.empty();
  }

  /// Checks wether this session is initialized (Handshake is done)
  bool is_initialized() {
    return SSL_is_init_finished(ssl_);
  }

  /// Returns a references to the write_buffer, containing data that should be
  /// be transmitted to the peer.
  util::byte_buffer& write_buffer() {
    return write_buf_;
  }

  /// Returns a reference to the encrypt buffer, containing data that should be
  /// encrypted.
  util::byte_buffer& encrypt_buffer() {
    return encrypt_buf_;
  }

  // -- interface functions ----------------------------------------------------

  /// Takes encrypted bytes from `bytes` and passes them to SSL. Possibly
  /// decrypted data is passed to the on_data callback.
  util::error consume(util::const_byte_span bytes);

  /// Takes plain bytes from `bytes` and passes them to SSL to be encrypted.
  /// Resulting encrypted bytes are copied to `encrypted_data_`.
  util::error encrypt(util::const_byte_span bytes);

private:
  util::error read_from_ssl();

  /// Handles the ssl-handshake.
  util::error handle_handshake();

  /// Queues plain bytes from `bytes` to be encrypted.
  void queue_encrypted_bytes(util::const_byte_span bytes);

  /// Queues encrypted bytes from `bytes` to be decrypted.
  void queue_plain_bytes(util::const_byte_span bytes);

  /// Denotes session to be either server, or client
  const session_type type_;
  /// SSL state
  SSL* ssl_;
  /// Write access to the SSL
  BIO* rbio_;
  /// Read access from the SSL
  BIO* wbio_;
  /// Bytes waiting to be written to socket
  util::byte_buffer write_buf_;
  /// Bytes waiting to be encrypted
  util::byte_buffer encrypt_buf_;
  /// Callback handling decrypted data
  on_data_callback_type on_data_;
  /// Buffer used for reading from SSL
  util::byte_array<2048> ssl_read_buf_;
};

} // namespace openssl
