/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include "net/event_result.hpp"
#include "net/layer.hpp"
#include "net/receive_policy.hpp"
#include "net/transport.hpp"

#include "util/error.hpp"

#include <utility>

namespace net {

/// Adapts the transport to the transport_extension layer
template <class NextLayer>
class transport_adaptor : public layer {
public:
  template <class... Ts>
  explicit transport_adaptor(transport& parent, Ts&&... xs)
    : parent_{parent}, next_layer_{*this, std::forward<Ts>(xs)...} {
  }

  util::error init() override {
    return next_layer_.init();
  }

  bool has_more_data() override {
    return next_layer_.has_more_data();
  }

  event_result produce() override {
    return next_layer_.produce();
  }

  event_result consume(util::const_byte_span bytes) override {
    return next_layer_.consume(bytes);
  }

  event_result handle_timeout(uint64_t id) override {
    return next_layer_.handle_timeout(id);
  }

  /// Configures the amount to be read next
  void configure_next_read(receive_policy policy) override {
    parent_.configure_next_read(policy);
  }

  /// Returns a reference to the send_buffer
  util::byte_buffer& write_buffer() override {
    return parent_.write_buffer();
  }

  /// Enqueues data to the transport extension
  void enqueue(util::const_byte_span bytes) override {
    parent_.enqueue(bytes);
  }

  /// Called when an error occurs
  void handle_error(util::error err) override {
    parent_.handle_error(err);
  }

  /// Registers the stack for write events
  void register_writing() override {
    parent_.register_writing();
  }

  /// Returns a reference to the following layer
  NextLayer& next_layer() {
    return next_layer_;
  }

private:
  transport& parent_;
  NextLayer next_layer_;
};

} // namespace net
