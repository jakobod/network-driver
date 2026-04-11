// /**
//  *  @author    Jakob Otto
//  *  @file      transport_adaptor.hpp
//  *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
//  *             This file is part of the network-driver project, released
//  under
//  *             the GNU GPL3 License.
//  */

// #pragma once

// #include "net/fwd.hpp"
// #include "util/fwd.hpp"

// #include "net/detail/layer_base.hpp"
// #include "net/detail/transport_base.hpp"

// #include "net/receive_policy.hpp"

// #include "util/error.hpp"

// #include <utility>

// namespace net::detail {

// /// @brief Adapter that connects a transport to a protocol layer stack.
// /// Bridges the transport layer with protocol layers above it, forwarding
// /// calls and delegating operations to the next layer in the protocol stack.
// /// @tparam NextLayer The upper protocol layer to bridge to.
// template <class NextLayer>
// class transport_adaptor : public layer_base {
// public:
//   /// @brief Constructs the adaptor with a transport and layer arguments.
//   /// @param parent The transport layer to adapt.
//   /// @param xs Constructor arguments forwarded to NextLayer.
//   template <class... Ts>
//   explicit transport_adaptor(transport_base& parent, Ts&&... xs)
//     : parent_{parent}, next_layer_{*this, std::forward<Ts>(xs)...} {}

//   /// @brief Initializes the protocol stack with configuration.
//   /// @param cfg The configuration object.
//   /// @return An error if initialization fails, success otherwise.
//   util::error init(const util::config& cfg) override {
//     return next_layer_.init(cfg);
//   }

//   /// @brief Checks if there is more data to send.
//   /// @return True if the next layer has pending data.
//   bool has_more_data() override { return next_layer_.has_more_data(); }

//   /// @brief Produces more data from the next layer.
//   /// @return The result of the produce operation.
//   event_result produce() override { return next_layer_.produce(); }

//   /// @brief Consumes received data in the next layer.
//   /// @param bytes The received bytes to process.
//   /// @return The result of the consume operation.
//   event_result consume(util::const_byte_span bytes) override {
//     return next_layer_.consume(bytes);
//   }

//   /// @brief Handles a timeout in the next layer.
//   /// @param id The timeout identifier.
//   /// @return The result of timeout handling.
//   event_result handle_timeout(uint64_t id) override {
//     return next_layer_.handle_timeout(id);
//   }

//   /// @brief Configures the next read operation on the transport.
//   /// @param policy The receive policy.
//   void configure_next_read(receive_policy policy) override {
//     parent_.configure_next_read(policy);
//   }

//   /// @brief Returns a reference to the transport's write buffer.
//   /// @return A reference to the output buffer.
//   util::byte_buffer& write_buffer() override { return parent_.write_buffer();
//   }

//   // /// @brief Enqueues data for transmission via the transport.
//   // /// @param bytes The bytes to enqueue.
//   // void enqueue(util::const_byte_span bytes) override {
//   // parent_.enqueue(bytes); }

//   // /// @brief Registers the stack for write readiness events.
//   // void register_writing() override {
//   //   // parent_.register_writing();
//   // }

//   // /// @brief Schedules a timeout after the specified duration.
//   // /// @param duration The timeout delay in milliseconds.
//   // /// @return A timeout identifier (0 if not implemented).
//   // uint64_t set_timeout_in(std::chrono::milliseconds) override {
//   //   return 0;
//   //   // return parent_.set_timeout_in(duration);
//   // }

//   // /// @brief Schedules a timeout at an absolute time point.
//   // /// @param point The time point when the timeout should occur.
//   // /// @return A timeout identifier (0 if not implemented).
//   // uint64_t set_timeout_at(std::chrono::system_clock::time_point) override
//   {
//   //   return 0;
//   //   // return parent_.set_timeout_at(point);
//   // }

//   /// @brief Returns a reference to the next layer in the stack.
//   /// @return A reference to the layer.
//   NextLayer& next_layer() { return next_layer_; }

// private:
//   transport_base& parent_;
//   NextLayer next_layer_;
// };

// } // namespace net::detail
