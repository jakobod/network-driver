/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "openssl/tls_session.hpp"

#include "util/error.hpp"
#include "util/format.hpp"

#include <openssl/err.h>

#include <cassert>

namespace {

/// Custom enum for relevant states of the SSL
enum ssl_status {
  ok,
  want_io,
  fail,
};

/// Returns the current status of SSL using the custom `ssl_status`
ssl_status get_status(SSL* ssl, int ret) {
  switch (SSL_get_error(ssl, ret)) {
    case SSL_ERROR_NONE:
      return ok;
    case SSL_ERROR_WANT_WRITE:
    case SSL_ERROR_WANT_READ:
      return want_io;
    default:
      return fail;
  }
}

} // namespace

namespace openssl {

using util::error_code::openssl_error;

/// Default buffer size for local buffers
static constexpr const std::size_t default_buf_size = 1024;

tls_session::tls_session(tls_context& ctx, util::byte_buffer& write_buffer,
                         on_data_callback_type on_data, session_type type)
  : type_{type},
    ssl_{SSL_new(ctx.context())},
    rbio_{BIO_new(BIO_s_mem())},
    wbio_{BIO_new(BIO_s_mem())},
    write_buf_{write_buffer},
    on_data_{std::move(on_data)} {
  // Set SSL to either client or server mode
  if (type_ == session_type::server)
    SSL_set_accept_state(ssl_);
  else if (type_ == session_type::client)
    SSL_set_connect_state(ssl_);
  SSL_set_bio(ssl_, rbio_, wbio_);
}

tls_session::~tls_session() {
  // SSL_free(ssl_); // Leads to double frees for some reason
}

util::error tls_session::init() {
  // Check the relevant ssl members
  if (!ssl_)
    return {openssl_error, "SSL object was not created"};
  if (!rbio_)
    return {openssl_error, "rBIO object was not created"};
  if (!wbio_)
    return {openssl_error, "wBIO object was not created"};
  // As client, initiate handshake
  if (type_ == session_type::client) {
    if (auto err = handle_handshake()) {
      return err;
    }
  }
  return util::none;
}

util::error tls_session::consume(util::const_byte_span bytes) {
  while (!bytes.empty()) {
    auto write_res = BIO_write(rbio_, bytes.data(), bytes.size());
    if (write_res <= 0)
      return {openssl_error, "BIO_read failed"};
    bytes = bytes.subspan(write_res);

    if (!is_initialized()) {
      if (auto err = handle_handshake())
        return err;
      if (!is_initialized())
        return util::none;
    }

    int read_res = 0;
    while (true) {
      read_res = SSL_read(ssl_, ssl_read_buf_.data(), ssl_read_buf_.size());
      if (read_res > 0)
        on_data_({ssl_read_buf_.data(), static_cast<size_t>(read_res)});
      else
        break;
    }

    switch (get_status(ssl_, read_res)) {
      case want_io:
        if (auto err = read_from_ssl())
          return err;
        return util::none;

      case fail:
        return {openssl_error, "SSL_read failed"};

      default:
        return util::none;
    }
  }
  return util::none;
}

util::error tls_session::encrypt(util::const_byte_span bytes) {
  queue_plain_bytes(bytes);

  // Wait for initialization to be done
  if (!SSL_is_init_finished(ssl_))
    return util::none;

  // Pass all queued plain bytes to SSL for encryption
  while (!encrypt_buf_.empty()) {
    auto write_res
      = SSL_write(ssl_, reinterpret_cast<const void*>(encrypt_buf_.data()),
                  static_cast<int>(encrypt_buf_.size()));
    if (write_res > 0) {
      encrypt_buf_.erase(encrypt_buf_.begin(),
                         encrypt_buf_.begin() + write_res);
    } else if (write_res < 0) {
      return {openssl_error, "SSL_write failed"};
    }
  }

  return read_from_ssl();
}

util::error tls_session::read_from_ssl() {
  // Retrieve all encrypted bytes from SSL.
  while (true) {
    auto read_res = BIO_read(wbio_,
                             reinterpret_cast<void*>(ssl_read_buf_.data()),
                             static_cast<int>(ssl_read_buf_.size()));
    if (read_res > 0)
      queue_encrypted_bytes(
        {ssl_read_buf_.data(), static_cast<std::size_t>(read_res)});
    else if (BIO_should_retry(wbio_))
      return util::none;
    else
      return {openssl_error, "BIO_read failed"};
  }
}

util::error tls_session::handle_handshake() {
  auto handshake_res = SSL_do_handshake(ssl_);
  switch (get_status(ssl_, handshake_res)) {
    case want_io:
      if (auto err = read_from_ssl())
        return err;
      return util::none;

    case fail:
      return {openssl_error, "SSL_do_handshake failed"};

    default:
      return util::none;
  }
}

void tls_session::queue_encrypted_bytes(util::const_byte_span bytes) {
  write_buf_.insert(write_buf_.end(), bytes.begin(), bytes.end());
}

void tls_session::queue_plain_bytes(util::const_byte_span bytes) {
  encrypt_buf_.insert(encrypt_buf_.end(), bytes.begin(), bytes.end());
}

tls_session make_client_session(tls_context& ctx,
                                util::byte_buffer& write_buffer,
                                tls_session::on_data_callback_type on_data) {
  return {ctx, write_buffer, std::move(on_data), session_type::client};
}

tls_session make_server_session(tls_context& ctx,
                                util::byte_buffer& write_buffer,
                                tls_session::on_data_callback_type on_data) {
  return {ctx, write_buffer, std::move(on_data), session_type::server};
}

} // namespace openssl
