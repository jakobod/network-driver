/**
 *  @author    Jakob Otto
 *  @file      udp.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <iostream>
#include <netinet/udp.h>

#include "detail/byte_container.hpp"

namespace packet {

/// @brief CRTP-based UDP protocol layer encapsulation.
/// Wraps a payload with a UDP header containing source and destination ports.
/// Uses CRTP to compose with other protocol layers (e.g., IP, Ethernet).
/// @tparam Payload The wrapped protocol layer (typically raw payload data).
template <class Payload>
class udp : public Payload {
public:
  // -- Types ------------------------------------------------------------------

  /// @brief The base class type.
  using super = Payload;

  // -- Const values -----------------------------------------------------------

  /// @brief Size of a UDP header in bytes (8 bytes).
  static constexpr auto header_length = sizeof(udphdr);

  // -- Constructors -----------------------------------------------------------

  /// @brief Constructs a UDP datagram with source and destination ports.
  /// Initializes the UDP header with port numbers and zeroes the checksum
  /// field. The payload layer is initialized with the remaining buffer space.
  /// @param data A byte span containing space for the UDP header and payload.
  /// @param src_port The source port number (in host byte-order, converted to
  /// network).
  /// @param dst_port The destination port number.
  /// @param xs Additional constructor arguments forwarded to the payload layer.
  template <class... Ts>
  explicit udp(detail::byte_span data, uint16_t src_port, uint16_t dst_port,
               Ts&&... xs)
    : super(data.subspan(header_length), std::forward<Ts>(xs)...) {
    udp_header_ = reinterpret_cast<udphdr*>(data.data());
    udp_header_->uh_sport = htons(src_port);
    udp_header_->uh_dport = htons(dst_port);
    udp_header_->len = htons(header_length);
    udp_header_->check = 0;
  }

  // -- Public API -----------------------------------------------------------

  /// @brief Returns a pointer to the UDP header structure.
  /// Allows direct access to read or modify UDP header fields.
  /// @return Pointer to the UDP header.
  udphdr* udp_header() { return udp_header_; }

  /// @brief Returns the total size of this UDP datagram (header + payload).
  /// Delegates to the payload's size() and adds the header length.
  /// @return The complete datagram size in bytes.
  size_t size() { return header_length + super::size(); }

  /// @brief Appends data to the encapsulated payload.
  /// Updates the UDP length field to reflect the new datagram size.
  /// @param payload The data to append.
  /// @return The new total datagram size.
  size_t append(detail::byte_span payload) {
    auto payload_size = super::append(payload);
    auto new_size = payload_size + header_length;
    udp_header_->len = htons(new_size);
    return new_size;
  }

private:
  udphdr* udp_header_; ///< Pointer to the UDP header
};

} // namespace packet
