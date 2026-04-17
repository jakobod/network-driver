/**
 *  @author    Jakob Otto
 *  @file      datagram_transport.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "net/detail/datagram_endpoint_wrapper.hpp"

#include <functional>
#include <ranges>
#include <unordered_map>

namespace net::detail {

template <class NextLayer>
class datagram_dispatcher {
  template <class Parent>
  struct parent_wrapper {
    Parent& parent_;
    datagram_dispatcher& self;

    void enqueue(util::byte_buffer&& buf, net::ip::v4_endpoint ep) {
      // dispatcher logic here, e.g., routing, framing
      parent_.enqueue(std::move(buf), ep);
    }

    void enqueue(util::const_byte_span buf, net::ip::v4_endpoint ep) {
      // dispatcher logic here, e.g., routing, framing
      parent_.enqueue(buf, ep);
    }

    void configure_next_read(receive_policy pol, net::ip::v4_endpoint) {
      parent_.configure_next_read(pol);
    }

    /// @brief Sets a timeout to trigger after the specified duration.
    /// @param in The duration to wait before the timeout fires
    /// @return A unique timeout identifier that can be used to cancel or
    /// identify the timeout
    uint64_t set_timeout_in(std::chrono::steady_clock::duration in,
                            net::ip::v4_endpoint ep) {
      const auto id = parent_.set_timeout_in(in);
      self.timeouts_.emplace(id, ep);
      return id;
    }

    /// @brief Sets a timeout to trigger at the specified absolute time point.
    /// @param when The absolute time point at which the timeout should fire
    /// @return A unique timeout identifier that can be used to cancel or
    /// identify
    ///         the timeout
    uint64_t set_timeout_at(std::chrono::steady_clock::time_point when,
                            net::ip::v4_endpoint ep) {
      const auto id = parent_.set_timeout_at(when);
      self.timeouts_.emplace(id, ep);
      return id;
    }

    // get_buffer()?
  };
  // explicit deduction guide
  template <class Parent>
  parent_wrapper(Parent&, datagram_dispatcher&) -> parent_wrapper<Parent>;

public:
  using application_factory_type = std::function<NextLayer()>;

  datagram_dispatcher(application_factory_type factory)
    : factory_{std::move(factory)} {
    // nop
  }

  util::error init(auto&, const util::config& cfg) {
    cfg_ = std::addressof(cfg);
    // parent.configure_next_read(
    //   receive_policy::up_to(datagram_size));
    return util::none;
  }

  manager_result produce(auto& parent) {
    parent_wrapper wrapper{parent, *this};
    for (auto& [_, layer] : layers_) {
      auto res = layer.produce(wrapper);
      if (res != manager_result::ok) {
        return res;
      }
    }
    return manager_result::ok;
  }

  // /// @brief Configures the read buffer for the next read operation.
  // /// @param policy The receive policy specifying min and max read sizes.
  // void configure_next_read(receive_policy policy) noexcept {
  //   // This should probably be handled by this layer too, but must be tracked
  //   // aditionally
  // }

  bool has_more_data() const noexcept {
    return std::any_of(layers_.begin(), layers_.end(),
                       [](const auto& p) { return p.second.has_more_data(); });
  }

  manager_result consume(auto& parent, util::const_byte_span data,
                         net::ip::v4_endpoint ep) {
    parent_wrapper wrapper{parent, *this};
    if (auto* layer = next_layer(wrapper, ep)) {
      return layer->consume(wrapper, data, ep);
    }
    return manager_result::error;
  }

  manager_result handle_timeout(auto& parent, uint64_t id) {
    auto timeout_ep = timeouts_.find(id);
    if (timeout_ep == timeouts_.end()) {
      return manager_result::error;
    }

    auto layer = layers_.find(timeout_ep->second);
    if (layer == layers_.end()) {
      return manager_result::error;
    }
    parent_wrapper wrapper{parent, *this};
    return layer->second.handle_timeout(wrapper, id);
  }

private:
  datagram_endpoint_wrapper<NextLayer>* next_layer(auto& wrapper,
                                                   net::ip::v4_endpoint ep) {
    auto [it, inserted] = layers_.try_emplace(ep, ep, factory_());
    if (inserted) {
      if ([[maybe_unused]] auto err = it->second.init(wrapper, *cfg_)) {
        LOG_ERROR("Failed to initialize new endpoint ", NET_ARG(err));
        return nullptr;
      }
    }
    return &it->second;
  }

  std::unordered_map<net::ip::v4_endpoint, datagram_endpoint_wrapper<NextLayer>>
    layers_;
  std::unordered_map<std::uint64_t, net::ip::v4_endpoint> timeouts_;
  const util::config* cfg_{nullptr};
  application_factory_type factory_;
};

} // namespace net::detail
