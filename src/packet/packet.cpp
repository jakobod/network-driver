/**
 *  @author    Jakob Otto
 *  @file      packet.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "packet/packet.hpp"

namespace udp {

packet::packet() : payload_size_(0) {
  memset(data_.data(), 0, data_.size());
  ethernet_header_ = reinterpret_cast<ether_header*>(data_.data());
  ip_header_ = reinterpret_cast<ip*>(data_.data() + ip_header_offset);
  udp_header_ = reinterpret_cast<udphdr*>(data_.data() + udp_header_offset);
  // Ethernet Header defaults
  ethernet_header_->ether_type = htons(ETH_P_IP);
  // IP header defaults
  ip_header_->ip_hl = 5; /* header length, number of 32-bit words */
  ip_header_->ip_v = 4;
  ip_header_->ip_tos = 0x0;
  ip_header_->ip_id = 12345;
  ip_header_->ip_off = htons(0x4000); /* Don't fragment */
  ip_header_->ip_ttl = 64;
  ip_header_->ip_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr));
  ip_header_->ip_p = IPPROTO_UDP;
  // UDP header defaults
  udp_header_->len = htons(sizeof(struct udphdr));
  udp_header_->check = 0;
}

} // namespace udp
