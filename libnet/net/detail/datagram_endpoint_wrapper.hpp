/**
 *  @author    Jakob Otto
 *  @file      datagram_transport.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "net/detail/datagram_transport.hpp"
#include "net/detail/transport_base.hpp"

#include <ranges>

namespace net::detail {

template <class NextLayer>
class datagram_endpoint_wrapper {
  template <class Parent>
  struct parent_wrapper {
    Parent& parent_;
    datagram_endpoint_wrapper& self;

    void enqueue(util::byte_buffer&& buf) {
      // dispatcher logic here, e.g., routing, framing
      parent_.enqueue(std::move(buf), self.ep_);
    }

    void enqueue(util::const_byte_span buf) {
      // dispatcher logic here, e.g., routing, framing
      parent_.enqueue(buf, self.ep_);
    }

    void configure_next_read(receive_policy pol) {
      parent_.configure_next_read(pol, self.ep_);
    }

    uint64_t set_timeout_in(std::chrono::steady_clock::duration in) {
      return parent_.set_timeout_in(in, self.ep_);
    }

    uint64_t set_timeout_at(std::chrono::steady_clock::time_point when) {
      return parent_.set_timeout_at(when, self.ep_);
    }
  };
  // explicit deduction guide
  template <class Parent>
  parent_wrapper(Parent&, datagram_endpoint_wrapper&) -> parent_wrapper<Parent>;

public:
  datagram_endpoint_wrapper(net::ip::v4_endpoint ep, NextLayer&& next_layer)
    : ep_{std::move(ep)}, next_layer_{std::move(next_layer)} {
    // nop
  }

  util::error init(auto& parent, const util::config& cfg) {
    parent_wrapper wrapper{parent, *this};
    return next_layer_.init(wrapper, cfg);
  }

  manager_result produce(auto& parent) {
    parent_wrapper wrapper{parent, *this};
    return next_layer_.produce(wrapper);
  }

  bool has_more_data() const noexcept { return next_layer_.has_more_data(); }

  manager_result consume(auto& parent, util::const_byte_span data,
                         net::ip::v4_endpoint ep) {
    if (ep_ != ep) {
      return manager_result::error;
    }
    parent_wrapper wrapper{parent, *this};
    return next_layer_.consume(wrapper, data);
  }

  manager_result handle_timeout(auto& parent, uint64_t id) {
    parent_wrapper wrapper{parent, *this};
    return next_layer_.handle_timeout(wrapper, id);
  }

private:
  net::ip::v4_endpoint ep_;
  NextLayer next_layer_;
};

} // namespace net::detail
