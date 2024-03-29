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

template <class Payload>
class ethernet : public Payload {
public:
  // -- Types ------------------------------------------------------------------

  using super = Payload;

  // -- Const values -----------------------------------------------------------

  static constexpr auto header_length = sizeof(ether_header);

  // -- Constructors -----------------------------------------------------------

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

  // -- Public API -------------------------------------------------------------

  ether_header* ethernet_header() {
    return ethernet_header_;
  }

  detail::byte_span as_span() {
    return {data_.data(), size()};
  }

  size_t size() {
    return header_length + super::size();
  }

  size_t append(detail::byte_span payload) {
    return super::append(payload);
  }

private:
  detail::byte_array<ETH_FRAME_LEN> data_;
  ether_header* ethernet_header_;
};

} // namespace packet
