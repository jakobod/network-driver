/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 02.03.2021
 */

#pragma once

#include <cstring>
#include <iostream>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <span>

#include "fwd.hpp"
#include "net/mac_address.hpp"
#include "net/socket_sys_includes.hpp"

namespace udp {

static constexpr auto eth_header_length = sizeof(ether_header);
static constexpr auto ip_header_length = sizeof(ip);
static constexpr auto udp_header_length = sizeof(udphdr);
static constexpr auto combined_header_length
  = eth_header_length + ip_header_length + udp_header_length;

static constexpr auto max_payload_size = ETH_FRAME_LEN - eth_header_length
                                         - ip_header_length - udp_header_length;

static constexpr auto ip_header_offset = eth_header_length;
static constexpr auto udp_header_offset = ip_header_offset + ip_header_length;
static constexpr auto payload_offset = udp_header_offset + udp_header_length;

class packet {
public:
  packet();

  ether_header& ethernet_header() {
    return *ethernet_header_;
  }

  ip& ip_header() {
    return *ip_header_;
  }

  udphdr& udp_header() {
    return *udp_header_;
  }

  detail::byte_span payload() {
    return std::span(data_.data() + payload_offset, max_payload_size);
  }

  void payload_size(size_t size) {
    payload_size_ = size;
  }

  detail::byte_array<ETH_FRAME_LEN> data() {
    return data_;
  }

  [[nodiscard]] size_t size() const noexcept {
    return combined_header_length + payload_size_;
  }

  packet& operator+=(detail::byte_span payload) {
    auto space_left = max_payload_size - payload_size_;
    if (payload.size() > space_left) {
      std::cerr << "payload too large! " << payload.size() << " vs. "
                << max_payload_size << " left";
      abort();
    }
    auto payload_ptr = data_.data() + payload_offset + payload_size_;
    memcpy(payload_ptr, payload.data(), payload.size());
    payload_size_ += payload.size();
    ip_header_->ip_len
      = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + payload_size_);
    udp_header_->len = htons(sizeof(struct udphdr) + payload_size_);
    return *this;
  }

  void set_src_mac(net::mac_address mac_addr) {
    memcpy(ethernet_header_->ether_shost, mac_addr.data(), mac_addr.size());
  }

  void set_dst_mac(net::mac_address mac_addr) {
    memcpy(ethernet_header_->ether_dhost, mac_addr.data(), mac_addr.size());
  }

  void set_src_addr(const std::string& address) {
    auto src_addr = to_sockaddr(address);
    ip_header().ip_src.s_addr = src_addr.sin_addr.s_addr;
    calculate_ip_checksum();
  }

  void set_dst_addr(const std::string& address) {
    auto dst_addr = to_sockaddr(address);
    ip_header().ip_dst.s_addr = dst_addr.sin_addr.s_addr;
    calculate_ip_checksum();
  }

  void set_src_port(uint16_t port) {
    udp_header_->source = htons(port);
  }

  void set_dst_port(uint16_t port) {
    udp_header_->dest = htons(port);
  }

  void calculate_ip_checksum() {
    ip_header_->ip_sum = 0;
    auto ptr = reinterpret_cast<uint16_t*>(ip_header_);
    auto len = static_cast<int>(sizeof(ip));
    unsigned long sum = 0;
    for (int nwords = len / 2; nwords > 0; nwords--)
      sum += *ptr++;
    if (len % 2 == 1)
      sum += *((unsigned char*) ptr);
    sum = (sum >> 16) + (sum & 0xffff);
    ip_header_->ip_sum = static_cast<unsigned short>(~sum);
  }

private:
  static sockaddr_in to_sockaddr(const std::string& address) {
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, address.c_str(), (in_addr*) &addr.sin_addr.s_addr);
    return addr;
  }

  detail::byte_array<ETH_FRAME_LEN> data_;
  ether_header* ethernet_header_;
  ip* ip_header_;
  udphdr* udp_header_;
  size_t payload_size_;
};

} // namespace udp
