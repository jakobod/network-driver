/**
 *  @author    Jakob Otto
 *  @file      datagram_transport.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

// #include "net/fwd.hpp"

// #include "net/detail/multiplexer_base.hpp"
// #include "net/detail/transport_base.hpp"

// #include "net/receive_policy.hpp"

// #include "net/socket/udp_datagram_socket.hpp"

// #include "util/byte_array.hpp"
// #include "util/error.hpp"

// #include <unordered_map>
// #include <utility>

// namespace net::detail {

// /// Implements a datagram oriented transport.
// template <class NextLayer>
// class datagram_transport : public transport_base {
//   static constexpr const std::size_t max_datagram_size = 576;

//   using layer_factory = std::function<NextLayer(transport_base*)>;

// public:
//   template <class... Ts>
//   datagram_transport(udp_datagram_socket handle, detail::multiplexer_base*
//   mpx,
//                      layer_factory layer_factory)
//     : transport_base{handle, mpx}, layer_factory_{std::move(layer_factory)} {
//     // nop
//   }

//   util::error init(const util::config& cfg) override {
//     if (auto err = transport_base::init(cfg)) {
//       return err;
//     }
//     transport_base::configure_next_read(
//       receive_policy::up_to(max_datagram_size));
//     return util::none;
//   }

//   // -- socket_manager API
//   -----------------------------------------------------

//   event_result handle_read_event() override {
//     for (size_t i = 0; i < max_consecutive_reads_; ++i) {
//       auto [source_ep, read_res] = read(handle<udp_datagram_socket>(),
//                                         read_buffer_);
//       if (read_res > 0) {
//         received_ += read_res;
//         if (received_ >= min_read_size_) {
//           if (next_layer(source_ep).consume(
//                 std::span(read_buffer_.data(), read_res))
//               == event_result::error) {
//             return event_result::error;
//           }
//         }
//       } else if (read_res < 0) {
//         if (!last_socket_error_is_temporary()) {
//           handle_error(util::error(util::error_code::socket_operation_failed,
//                                    "[socket_manager.read()] "
//                                      + last_socket_error_as_string()));
//           return event_result::error;
//         }
//         return event_result::ok;
//       } else {
//         // Empty packets are valid, but can be ignored
//       }
//     }
//     return event_result::ok;
//   }

//   event_result handle_write_event() override {
//     // Checks if this transport is done writing
//     auto done_writing = [this](const NextLayer& layer) -> bool {
//       return (write_buffer_.empty() && !layer.has_more_data());
//     };

//     auto fetch = [this](const NextLayer& layer) -> bool {
//       auto fetched = layer.has_more_data();
//       for (size_t i = 0;
//            layer.has_more_data() && (i < max_consecutive_fetches_); ++i) {
//         layer.produce(*this);
//       }
//       return fetched;
//     };

//     if (write_buffer_.empty()) {
//       if (!fetch()) {
//         return event_result::done;
//       }
//     }
//     for (size_t i = 0; i < max_consecutive_writes_; ++i) {
//       const auto num_bytes = write_buffer_.size() - written_;
//       const auto write_res = write(handle<udp_datagram_socket>(),
//                                    std::span(write_buffer_.data(),
//                                    num_bytes));
//       if (write_res > 0) {
//         write_buffer_.erase(write_buffer_.begin(),
//                             write_buffer_.begin() + write_res);
//         if (write_buffer_.empty()) {
//           if (!fetch()) {
//             return event_result::done;
//           }
//         }
//       } else {
//         if (last_socket_error_is_temporary()) {
//           return event_result::ok;
//         } else {
//           handle_error(util::error(util::error_code::socket_operation_failed,
//                                    "[datagram_transport::write()]"
//                                      + last_socket_error_as_string()));
//           return event_result::error;
//         }
//       }
//     }
//     return done_writing() ? event_result::done : event_result::ok;
//   }

//   event_result handle_timeout(uint64_t) {
//     return event_result::ok;
//     // return application_.handle_timeout(*this, id);
//   }

// private:
//   NextLayer& next_layer(const ip::v4_endpoint& ep) {
//     auto it = layers_.find(source_ep);
//     if (it == layers_.end()) {
//       auto emplace_result = layers_.emplace(ep, layer_factory_(this));
//       it = emplace_result.first;
//     }
//     return *it;
//   }

//   layer_factory layer_factory_;
//   std::unordered_map<ip::v4_endpoint, NextLayer> layers_;
// };

// } // namespace net::detail
