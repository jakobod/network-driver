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

/// Server side connection session. Each time a connection is accepted, this
/// session is constructed and used for en-/decrypting
struct server_session {
  using util::error_code::openssl_error;

public:
  server_session(SSL_CTX* ctx)
    : ssl_{SSL_new(ctx)},
      rbio_{BIO_new(BIO_s_mem())},
      wbio_{BIO_new(BIO_s_mem())} {
    // Sets ssl to work in server mode.
    SSL_set_accept_state(ssl_);
    SSL_set_bio(ssl_, rbio_, wbio_);
  }

  ~server_session() {
    SSL_free(ssl_);
  }

  bool has_more_data() {
    return !write_buf_.empty();
  }

  ssl_status get_ssl_status(int n) {
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

  /* Queue encrypted bytes for socket write. Should only be used when the SSL
   * object has requested a write operation. */
  void queue_encrypted_bytes(util::const_byte_span bytes) {
    write_buf_.insert(write_buf_.end(), bytes.begin(), bytes.end());
  }

  /// Takes in encrypted bytes, decrypts them, and passes it to a callback
  util::error consume_encrypted(util::const_byte_span bytes) {
    util::byte_array<512> buf;

    while (!bytes.empty()) {
      auto ret = BIO_write(rbio_, bytes.data(), bytes.size());
      if (ret <= 0)
        return util::error(openssl_error, "BIO_write failed");
      bytes = bytes.subspan(ret);

      if (!SSL_is_init_finished(ssl_)) {
        switch (get_sslstatus(SSL_accept(ssl_))) {
          case ssl_status::want_io:
            while (true) {
              if (auto ret = BIO_read(wbio_, buf.data(), buf.size()); ret > 0)
                queue_encrypted_bytes(
                  {buf.data(), static_cast<std::size_t>(ret)});
              else if (BIO_should_retry(wbio_))
                break;
              else
                return util::error(openssl_error, "BIO_read failed");
            }
            break;
          case ssl_status::fail:
            return util::error(openssl_error, "ssl_status::fail");
          default:
            break;
        }

        if (!SSL_is_init_finished(ssl_))
          return util::none;
      }

      // -- encrypted data should be decrypted now -----------------------------

      // Receive some data and pass it on
      ret = 0;
      while (true) {
        ret = SSL_read(ssl_, buf.data(), buf.size());
        if (ret > 0)
          on_decrypted_data({buf.data(), static_cast<std::size_t>(ret)});
        else
          break;
      }

      // check status again and write if needed
      switch (get_sslstatus(ret)) {
        /* Did SSL request to write bytes? This can happen if peer has requested
         * SSL renegotiation. */
        case ssl_status::want_io:
          while (true) {
            auto ret = BIO_read(wbio_, buf.data(), buf.size());
            if (ret > 0)
              queue_encrypted_bytes(
                {buf.data(), static_cast<std::size_t>(ret)});
            else if (BIO_should_retry(wbio_))
              break;
            else
              return util::error(openssl_error, "BIO_read failed");
          }
          break;

        case ssl_status::fail:
          return util::error(openssl_error, "openssl status fail");

        default:
          break;
      }
    }

    return util::none;
  }

  /* Process outbound unencrypted data that are waiting to be encrypted.  The
   * waiting data resides in encrypt_buf.  It needs to be passed into the SSL
   * object for encryption, which in turn generates the encrypted bytes that
   * then will be queued for later socket write. */
  util::error do_encrypt() {
    util::byte_array<512> buf;

    // Wait for initialization to be done
    if (!SSL_is_init_finished(ssl_))
      return util::none;

    while (!encrypt_buf_.empty()) {
      auto res = SSL_write(ssl_, encrypt_buf_.data(), encrypt_buf_.size());

      if (get_sslstatus(res) == ssl_status::fail) {
        return util::error(openssl_error, "ssl_status fail");
      } else if (res == 0) {
        return util::none;
      } else if (res > 0) {
        /* consume the waiting bytes that have been used by SSL */
        if ((size_t) res < encrypt_buf_.size())
          encrypt_buf_.erase(encrypt_buf_.begin(), encrypt_buf_.begin() + res);

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
  }

private:
  void on_decrypted_data(util::const_byte_span bytes) {
    std::string str(reinterpret_cast<const char*>(bytes.data(), bytes.size()));
    std::cout << ">>> server received: " << str << std::endl;
  }

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
