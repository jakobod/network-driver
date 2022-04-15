/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/fwd.hpp"

#include "net/event_result.hpp"
#include "net/layer.hpp"
#include "net/receive_policy.hpp"

#include "openssl/tls_context.hpp"

#include "util/byte_array.hpp"
#include "util/error.hpp"

namespace net {

template <class NextLayer>
class tls : public layer {
  /// Custom enum for relevant states of the SSL
  enum ssl_status {
    ok,
    want_io,
    fail,
  };

  static constexpr const std::size_t buffer_size = 2048;

public:
  template <class... Ts>
  tls(layer& parent, openssl::tls_context& ctx, bool is_client, Ts&&... xs)
    : parent_{parent},
      next_layer_{*this, std::forward<Ts>(xs)...},
      is_client_{is_client},
      ssl_{SSL_new(ctx.context())},
      rbio_{BIO_new(BIO_s_mem())},
      wbio_{BIO_new(BIO_s_mem())} {
    // Set SSL to either client or server mode
    if (is_client)
      SSL_set_connect_state(ssl_);
    else
      SSL_set_accept_state(ssl_);
    SSL_set_bio(ssl_, rbio_, wbio_);
  }

  // -- Upfacing interface (towards application) -------------------------------

  util::error init() override {
    // Check the relevant ssl members
    if (!ssl_)
      return {util::error_code::openssl_error, "SSL object was not created"};
    if (!rbio_)
      return {util::error_code::openssl_error, "rBIO object was not created"};
    if (!wbio_)
      return {util::error_code::openssl_error, "wBIO object was not created"};
    // As client, initiate handshake
    if (is_client_) {
      if (auto err = handle_handshake()) {
        return err;
      }
    }
    parent_.configure_next_read(receive_policy::up_to(buffer_size));
    return next_layer_.init();
  }

  bool has_more_data() override {
    return (!encrypt_buf_.empty() || next_layer_.has_more_data());
  }

  event_result produce() override {
    if (next_layer_.produce() == event_result::error)
      return event_result::error;
    if (auto err = encrypt()) {
      parent_.handle_error(err);
      return event_result::error;
    }
    return !next_layer_.has_more_data() ? event_result::done : event_result::ok;
  }

  /// Takes received data from the transport and consumes it
  event_result consume(util::const_byte_span bytes) override {
    while (!bytes.empty()) {
      auto write_res = BIO_write(rbio_,
                                 reinterpret_cast<const void*>(bytes.data()),
                                 static_cast<int>(bytes.size()));
      if (write_res <= 0) {
        parent_.handle_error(
          util::error{util::error_code::openssl_error, "BIO_read failed"});
        return event_result::error;
      }
      bytes = bytes.subspan(write_res);

      if (!handshake_done()) {
        if (auto err = handle_handshake()) {
          parent_.handle_error(err);
          return event_result::error;
        }
        if (!handshake_done())
          return event_result::ok;
      }

      int read_res = 0;
      while (true) {
        read_res = SSL_read(ssl_, ssl_read_buf_.data(), ssl_read_buf_.size());
        if (read_res > 0)
          next_layer_.consume(util::const_byte_span{
            ssl_read_buf_.data(), static_cast<size_t>(read_res)});
        else
          break;
      }

      switch (get_status(read_res)) {
        case want_io:
          if (auto err = read_all_from_ssl()) {
            parent_.handle_error(err);
            return event_result::error;
          }
          return event_result::ok;

        case fail:
          parent_.handle_error(
            util::error{util::error_code::openssl_error, "SSL_read failed"});
          return event_result::error;

        default:
          return event_result::ok;
      }
    }
    return event_result::ok;
  }

  event_result handle_timeout(uint64_t id) override {
    return next_layer_.handle_timeout(id);
  }

  // -- Downfacing interface (towards transport) -------------------------------

  /// Configures the amount to be read next
  void configure_next_read(receive_policy) override {
    // Currently ignored for following applications.
  }

  /// Returns a reference to the send_buffer
  util::byte_buffer& write_buffer() override {
    return encrypt_buf_;
  }

  /// Enqueues data to the transport extension
  void enqueue(util::const_byte_span bytes) override {
    encrypt_buf_.insert(encrypt_buf_.end(), bytes.begin(), bytes.end());
  }

  /// Called when an error occurs
  void handle_error(const util::error& err) override {
    parent_.handle_error(err);
  }

  /// Registers the stack for write events
  void register_writing() override {
    parent_.register_writing();
  }

  /// Sets a timeout in `duration` milliseconds with the id `timeout_id`
  uint64_t set_timeout_in(std::chrono::milliseconds duration) override {
    return parent_.set_timeout_in(duration);
  }

  /// Sets a timeout at timepoint `point` with the id `timeout_id`
  uint64_t
  set_timeout_at(std::chrono::system_clock::time_point point) override {
    return parent_.set_timeout_at(point);
  }

  /// Checks wether this session is initialized (Handshake is done)
  bool handshake_done() {
    return SSL_is_init_finished(ssl_);
  }

private:
  util::error encrypt() {
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
        return {util::error_code::openssl_error, "SSL_write failed"};
      }
    }

    return read_all_from_ssl();
  }

  util::error read_all_from_ssl() {
    // Retrieve all encrypted bytes from SSL.
    while (true) {
      auto read_res = BIO_read(wbio_,
                               reinterpret_cast<void*>(ssl_read_buf_.data()),
                               static_cast<int>(ssl_read_buf_.size()));
      if (read_res > 0)
        parent_.enqueue(
          {ssl_read_buf_.data(), static_cast<std::size_t>(read_res)});
      else if (BIO_should_retry(wbio_))
        return util::none;
      else
        return {util::error_code::openssl_error, "BIO_read failed"};
    }
  }

  util::error handle_handshake() {
    auto handshake_res = SSL_do_handshake(ssl_);
    switch (get_status(handshake_res)) {
      case want_io:
        if (auto err = read_all_from_ssl())
          return err;
        return util::none;

      case fail:
        return {util::error_code::openssl_error, "SSL_do_handshake failed"};

      default:
        return util::none;
    }
  }

  /// Returns the current status of SSL using the custom `ssl_status`
  ssl_status get_status(int ret) {
    switch (SSL_get_error(ssl_, ret)) {
      case SSL_ERROR_NONE:
        return ok;
      case SSL_ERROR_WANT_WRITE:
      case SSL_ERROR_WANT_READ:
        return want_io;
      default:
        return fail;
    }
  }

  // Reference to the parent transport
  layer& parent_;
  // NextLayer
  NextLayer next_layer_;

  /// Denotes session to be either server, or client
  const bool is_client_;
  /// SSL state
  SSL* ssl_;
  /// Write access to the SSL
  BIO* rbio_;
  /// Read access from the SSL
  BIO* wbio_;
  /// Bytes waiting to be encrypted
  util::byte_buffer encrypt_buf_;

  /// Buffer used for reading from SSL
  util::byte_array<buffer_size> ssl_read_buf_;
};

} // namespace net
