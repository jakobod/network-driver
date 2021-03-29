/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 04.03.2021
 */

#pragma once

#include <iostream>
#include <netinet/udp.h>

#include "detail/byte_container.hpp"

namespace packet {

template <class Payload>
class udp : public Payload {
public:
  // -- Types ------------------------------------------------------------------

  using super = Payload;

  // -- Const values -----------------------------------------------------------

  static constexpr auto header_length = sizeof(udphdr);

  // -- Constructors -----------------------------------------------------------

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

  // -- Public API -------------------------------------------------------------

  udphdr* udp_header() {
    return udp_header_;
  }

  size_t size() {
    return header_length + super::size();
  }

  size_t append(detail::byte_span payload) {
    auto payload_size = super::append(payload);
    auto new_size = payload_size + header_length;
    udp_header_->len = htons(new_size);
    return new_size;
  }

private:
  udphdr* udp_header_;
};

} // namespace packet
