/**
 *  @author    Jakob Otto
 *  @file      ip.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "util/byte_container.hpp"

#include <iostream>
#include <netinet/ip.h>

namespace packet {

template <class Payload>
class ip : public Payload {
public:
  // -- Types ------------------------------------------------------------------

  using super = Payload;

  // -- Const values -----------------------------------------------------------

  static constexpr auto header_length = sizeof(::ip);

  // -- Constructors -----------------------------------------------------------

  template <class... Ts>
  ip(detail::byte_span data, std::string src, std::string dst,
     uint8_t ip_protocol, Ts&&... xs)
    : super(data.subspan(header_length), std::forward<Ts>(xs)...) {
    ip_header_ = reinterpret_cast<::ip*>(data.data());
    ip_header_->ip_hl = 5;
    ip_header_->ip_v = 4;
    ip_header_->ip_tos = 0x0;
    ip_header_->ip_id = 12345;
    ip_header_->ip_off = htons(0x4000);
    ip_header_->ip_ttl = 64;
    ip_header_->ip_len = htons(header_length + super::size());
    ip_header_->ip_p = ip_protocol;
    ip_header_->ip_sum = 0;
    auto to_sockaddr = [](const std::string& address) -> sockaddr_in {
      sockaddr_in addr = {};
      addr.sin_family = AF_INET;
      inet_pton(AF_INET, address.c_str(), (in_addr*) &addr.sin_addr.s_addr);
      return addr;
    };
    ip_header_->ip_src.s_addr = to_sockaddr(src).sin_addr.s_addr;
    ip_header_->ip_dst.s_addr = to_sockaddr(dst).sin_addr.s_addr;
    calculate_ip_checksum();
  }

  // -- Public API -------------------------------------------------------------

  ip* ip_header() {
    return ip_header_;
  }

  void calculate_ip_checksum() {
    ip_header_->ip_sum = 0;
    auto ptr = reinterpret_cast<uint16_t*>(ip_header_);
    auto len = static_cast<int>(header_length);
    unsigned long sum = 0;
    for (int nwords = len / 2; nwords > 0; nwords--)
      sum += *ptr++;
    if (len % 2 == 1)
      sum += *(reinterpret_cast<unsigned char*>(ptr));
    sum = (sum >> 16) + (sum & 0xffff);
    ip_header_->ip_sum = static_cast<unsigned short>(~sum);
  }

  size_t size() {
    return header_length + super::size();
  }

  size_t append(detail::byte_span payload) {
    auto payload_size = super::append(payload);
    auto new_size = payload_size + header_length;
    ip_header_->ip_len = htons(new_size);
    calculate_ip_checksum();
    return new_size;
  }

private:
  ::ip* ip_header_;
};

} // namespace packet
