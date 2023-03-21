/**
 *  @author    Jakob Otto
 *  @file      tls_context.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "util/fwd.hpp"

#include <openssl/ssl.h>

#include <memory>
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

  SSL_CTX* context() const noexcept {
    return ctx_;
  }

private:
  SSL_CTX* ctx_;
};

/// Shared tls_context ptr
using tls_context_ptr = std::shared_ptr<tls_context>;

} // namespace openssl
