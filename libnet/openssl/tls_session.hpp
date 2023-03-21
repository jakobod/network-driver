/**
 *  @author    Jakob Otto
 *  @file      tls_session.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "openssl/tls_context.hpp"

#include "util/byte_array.hpp"
#include "util/byte_buffer.hpp"
#include "util/byte_span.hpp"
#include "util/error.hpp"

#include <openssl/ssl.h>

#include <functional>
#include <iostream>

namespace openssl {

/// Denotes the type of session
enum class session_type {
  server,
  client,
};

/// TLS-session. Abstracts the en/decrypting of data
class tls_session {
  static constexpr const std::size_t buffer_size = 2048;

public:
  /// Callback type for on_data callback
  using on_data_callback_type = std::function<void(util::const_byte_span)>;

  // -- Constructors/Destructors/initialization --------------------------------

  /// Constructs a TLS server session
  tls_session(tls_context& ctx, util::byte_buffer& write_buffer,
              on_data_callback_type on_data, session_type type);

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

  // -- interface functions ----------------------------------------------------

  /// Takes encrypted bytes from `bytes` and passes them to SSL. Possibly
  /// decrypted data is passed to the on_data callback.
  util::error consume(util::const_byte_span bytes);

  /// Takes plain bytes from `bytes` and passes them to SSL to be encrypted.
  /// Resulting encrypted bytes are copied to `encrypted_data_`.
  util::error encrypt(util::const_byte_span bytes);

  util::byte_buffer& write_buffer() {
    return write_buf_;
  }

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
  util::byte_buffer& write_buf_;
  /// Bytes waiting to be encrypted
  util::byte_buffer encrypt_buf_;
  /// Callback handling decrypted data
  on_data_callback_type on_data_;
  /// Buffer used for reading from SSL
  util::byte_array<buffer_size> ssl_read_buf_;
};

/// Creates a client session from the given arguments
tls_session make_client_session(tls_context& ctx,
                                util::byte_buffer& write_buffer,
                                tls_session::on_data_callback_type on_data);

/// Creates a server session from the given arguments
tls_session make_server_session(tls_context& ctx,
                                util::byte_buffer& write_buffer,
                                tls_session::on_data_callback_type on_data);

} // namespace openssl
