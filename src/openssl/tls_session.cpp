/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "openssl/tls_context.hpp"

#include "util/error.hpp"
#include "util/format.hpp"

#include <openssl/err.h>

#include <cassert>

namespace openssl {

using util::error_code::openssl_error;

tls_session::tls_session(SSL_CTX* ctx, server_mode)
  : me{"server"},
    ssl_{SSL_new(ctx)},
    rbio_{BIO_new(BIO_s_mem())},
    wbio_{BIO_new(BIO_s_mem())} {
  assert(ssl_ != nullptr);
  assert(rbio_ != nullptr);
  assert(wbio_ != nullptr);

  // Sets ssl to work in server mode.
  SSL_set_accept_state(ssl_);
  SSL_set_bio(ssl_, rbio_, wbio_);
}

tls_session::tls_session(SSL_CTX* ctx, client_mode)
  : me{"client"},
    ssl_{SSL_new(ctx)},
    rbio_{BIO_new(BIO_s_mem())},
    wbio_{BIO_new(BIO_s_mem())} {
  assert(ssl_ != nullptr);
  assert(rbio_ != nullptr);
  assert(wbio_ != nullptr);

  // Sets ssl to work in server mode.
  SSL_set_connect_state(ssl_);
  SSL_set_bio(ssl_, rbio_, wbio_);
}

tls_session::~tls_session() {
  // SSL_free(ssl_);
}

bool tls_session::has_more_data() {
  return !write_buf_.empty();
}

void tls_session::queue_encrypted_bytes(util::const_byte_span bytes) {
  write_buf_.insert(write_buf_.end(), bytes.begin(), bytes.end());
}

void tls_session::queue_plain_bytes(util::const_byte_span bytes) {
  encrypt_buf_.insert(encrypt_buf_.end(), bytes.begin(), bytes.end());
}

util::error tls_session::handle_handshake() {
  util::byte_array<1024> buf;

  int ret = SSL_do_handshake(ssl_);
  if (auto status = get_ssl_status(ret); status == ssl_status::fail) {
    auto err = ERR_get_error();
    return {openssl_error,
            util::format("SSL connect err code:[{0}]({1}), REASON: {2}", err,
                         std::string(ERR_error_string(err, NULL)),
                         ERR_reason_error_string(err))};
  } else if (status == ssl_status::want_io) {
    while (true) {
      if (auto ret = BIO_read(wbio_, buf.data(), buf.size()); ret > 0) {
        queue_encrypted_bytes({buf.data(), static_cast<std::size_t>(ret)});
      } else if (BIO_should_retry(wbio_)) {
        break;
      } else {
        return {openssl_error, "BIO_read failed"};
      }
    }
  }
  return util::none;
}

util::error tls_session::consume_encrypted(util::const_byte_span bytes) {
  util::byte_array<1024> buf;

  ssl_status status;
  int n;

  while (!bytes.empty()) {
    n = BIO_write(rbio_, bytes.data(), bytes.size());

    if (n <= 0)
      return util::error(openssl_error, "BIO_read failed");
    bytes = bytes.subspan(n);

    if (!SSL_is_init_finished(ssl_)) {
      if (auto err = handle_handshake())
        return err;
      if (!SSL_is_init_finished(ssl_))
        return util::none;
    }

    /* The encrypted data is now in the input bio so now we can perform actual
     * read of unencrypted data. */

    do {
      n = SSL_read(ssl_, buf.data(), buf.size());
      if (n > 0)
        on_data({buf.data(), static_cast<size_t>(n)});
    } while (n > 0);

    status = get_ssl_status(n);

    /* Did SSL request to write bytes? This can happen if peer has requested SSL
     * renegotiation. */
    if (status == ssl_status::want_io)
      do {
        n = BIO_read(wbio_, buf.data(), buf.size());
        if (n > 0)
          queue_encrypted_bytes({buf.data(), static_cast<size_t>(n)});
        else if (!BIO_should_retry(wbio_))
          return util::error(openssl_error, "BIO_read failed");
      } while (n > 0);

    if (status == ssl_status::fail)
      return util::error(openssl_error, "SSL_read failed");
  }

  return util::none;
}

/* Process outbound unencrypted data that are waiting to be encrypted.  The
 * waiting data resides in encrypt_buf.  It needs to be passed into the SSL
 * object for encryption, which in turn generates the encrypted bytes that
 * then will be queued for later socket write. */
util::error tls_session::produce(util::const_byte_span bytes) {
  queue_plain_bytes(std::move(bytes));

  std::cout << me << " queued data" << std::endl;

  util::byte_array<1024> buf;

  // Wait for initialization to be done
  if (!SSL_is_init_finished(ssl_))
    return util::none;
  std::cout << me << " init finished" << std::endl;

  while (!encrypt_buf_.empty()) {
    auto res = SSL_write(ssl_, encrypt_buf_.data(),
                         static_cast<int>(encrypt_buf_.size()));

    if (get_ssl_status(res) == ssl_status::fail) {
      return util::error(openssl_error, "ssl_status fail");
    } else if (res == 0) {
      return util::none;
    } else if (res > 0) {
      /* consume the waiting bytes that have been used by SSL */
      if ((size_t) res < encrypt_buf_.size())
        encrypt_buf_.erase(encrypt_buf_.begin(), encrypt_buf_.begin() + res);
      else
        encrypt_buf_.clear();
      /* take the output of the SSL object and queue it for socket write */
      while (true) {
        res = BIO_read(wbio_, buf.data(), buf.size());
        if (res > 0)
          queue_encrypted_bytes({buf.data(), static_cast<std::size_t>(res)});
        else if (BIO_should_retry(wbio_))
          break;
        else
          return util::error(openssl_error, "BIO_read failed");
      }
    }
  }
  return util::none;
}

ssl_status tls_session::get_ssl_status(int n) {
  switch (SSL_get_error(ssl_, n)) {
    case SSL_ERROR_NONE:
      return ssl_status::ok;
    case SSL_ERROR_WANT_WRITE:
    case SSL_ERROR_WANT_READ:
      return ssl_status::want_io;
    case SSL_ERROR_ZERO_RETURN:
    case SSL_ERROR_SYSCALL:
    default:
      return ssl_status::fail;
  }
}

} // namespace openssl
