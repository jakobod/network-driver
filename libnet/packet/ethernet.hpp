/**
 *  @author    Jakob Otto
 *  @file      ethernet.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/mac_address.hpp"

#include "util/byte_array.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <net/ethernet.h>
#include <string>

namespace packet {

/// @brief CRTP-based Ethernet frame layer encapsulation.
/// Wraps a payload with an Ethernet header containing MAC addresses and
/// an EtherType field. Uses the curiously recurring template pattern to
/// compose with other protocol layers (e.g., IP, UDP).
/// @tparam Payload The wrapped protocol layer (typically an IP or raw payload
/// class).
template <class Payload>
class ethernet : public Payload {
public:
  // -- Types ------------------------------------------------------------------

  /// @brief The base class type.
  using super = Payload;

  // -- Const values -----------------------------------------------------------

  /// @brief Size of an Ethernet frame header in bytes (14 bytes).
  static constexpr auto header_length = sizeof(ether_header);

  // -- Constructors -----------------------------------------------------------

  /// @brief Constructs an Ethernet frame with source/destination MAC addresses.
  /// Initializes the Ethernet header with the provided MAC addresses and
  /// EtherType, then constructs the payload with the remaining buffer space.
  /// @param src The source MAC address.
  /// @param dst The destination MAC address.
  /// @param type The EtherType value (in host byte-order, converted to
  /// network).
  /// @param xs Additional constructor arguments forwarded to the payload layer.
  template <class... Ts>
  ethernet(net::mac_address src, net::mac_address dst, uint16_t type,
           Ts&&... xs)
    : super({data_.data() + header_length, data_.size() - header_length},
            std::forward<Ts>(xs)...),
      ethernet_header_(reinterpret_cast<ether_header*>(data_.data())) {
    memcpy(ethernet_header_->ether_shost, src.data(), src.size());
    memcpy(ethernet_header_->ether_dhost, dst.data(), dst.size());
    ethernet_header_->ether_type = htons(type);
  }

  // -- Public API -----------------------------------------------------------

  /// @brief Returns a pointer to the Ethernet header structure.
  /// Allows direct access to modify header fields.
  /// @return Pointer to the ether_header structure.
  ether_header* ethernet_header() { return ethernet_header_; }

  /// @brief Returns the complete frame (header + payload) as a byte span.
  /// @return A span covering the entire frame data.
  detail::byte_span as_span() { return {data_.data(), size()}; }

  /// @brief Returns the total size of this frame (header + payload).
  /// Delegates to the payload's size() and adds the header length.
  /// @return The complete frame size in bytes.
  size_t size() { return header_length + super::size(); }

  /// @brief Appends data to the encapsulated payload.
  /// Delegates directly to the payload layer's append method.
  /// @param payload The data to append.
  /// @return The new payload size, or -1 on insufficient space.
  size_t append(detail::byte_span payload) { return super::append(payload); }

private:
  detail::byte_array<ETH_FRAME_LEN> data_; ///< Full Ethernet frame buffer
  ether_header* ethernet_header_;          ///< Pointer to the Ethernet header
};

} // namespace packet
