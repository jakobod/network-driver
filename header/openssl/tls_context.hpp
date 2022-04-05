/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include "openssl/tls_session.hpp"

#include <openssl/ssl.h>

#include <string>

namespace openssl {

class tls_context {
public:
  /// Constructs the tls_context object
  tls_context();

  /// Destructs the tls_context object
  ~tls_context();

  /// Initializes and validates the tls_context object
  util::error init(const std::string& cert_path, const std::string& key_path);

  /// Constructs and returns a new server-side session object
  util::error_or<tls_session>
  new_client_session(tls_session::on_data_callback_type on_data);

  /// Constructs and returns a new server-side session object
  util::error_or<tls_session>
  new_server_session(tls_session::on_data_callback_type on_data);

private:
  SSL_CTX* ctx_;
};

} // namespace openssl
