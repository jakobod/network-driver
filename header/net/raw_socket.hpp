/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 18.02.2021
 */

#pragma once

#include <linux/if_packet.h>

#include "detail/error.hpp"
#include "fwd.hpp"
#include "net/socket.hpp"

namespace net {

struct raw_socket : socket {
  using super = socket;

  using super::super;
};

detail::error_or<raw_socket> make_raw_socket();

ssize_t sendto(raw_socket sock, sockaddr_ll socket_address,
               detail::byte_span data);

template <class Packet>
ssize_t sendto(raw_socket sock, sockaddr_ll socket_address, Packet& p) {
  return sendto(sock, socket_address, p.as_span());
}

} // namespace net

/*
TODO: This would be the routine of sending raw packets on Macos
// linux#include <sys/types.h>
#  include <sys/uio.h>
#  include <unistd.h>

#  include <cstdio>
#  include <cstdlib>
#  include <cstring>
#  include <fcntl.h>
#  include <numeric>

#  include <arpa/inet.h>
#  include <sys/ioctl.h>
#  include <sys/socket.h>

#  include <net/bpf.h>
#  include <net/if.h>
#  include <net/net.h>

#  include <iostream>
#  include <span>
#  include <vector>

#  include "detail/socket_guard.hpp"
#  include "net/frame.hpp"

// Fill in your source and destination MAC address
unsigned char dest_mac[ETHER_ADDR_LEN] = {0xe4, 0x54, 0xe8, 0x81, 0xbc, 0x5f};
unsigned char src_mac[ETHER_ADDR_LEN] = {0x78, 0x7b, 0x8a, 0xaf, 0x67, 0x9a};

// Try to open the bpf device
int open_dev() {
  for (int i = 0; i < 99; i++) {
    std::string device = "/dev/bpf" + std::to_string(i);
    if (auto bpf = open(device.c_str(), O_RDWR); bpf != -1) {
      std::cerr << "[open_dev] Opened device " << device << std::endl;
      return bpf;
    }
  }
  return invalid_fd;
}

// Associate bpf device with a physical net interface
bool assoc_dev(int bpf, std::string& interface) {
  ifreq bound_if = {};
  strcpy(bound_if.ifr_name, interface.c_str());
  if (ioctl(bpf, BIOCSETIF, &bound_if) > 0) {
    std::cerr << "[assoc_dev] Cannot bind bpf device to physical device "
              << interface << ", exiting" << std::endl;
    return false;
  }
  std::cerr << "[assoc_dev] Bound bpf device to physical device "
            << interface << std::endl;
  return true;
}

// Set some options on the bpf device, then get the length of the kernel buffer
int get_buf_len(int bpf) {
  int buf_len = 1;
  // activate immediate mode (therefore, buf_len is initially set to "1")
  if (ioctl(bpf, BIOCIMMEDIATE, &buf_len) == -1) {
    std::cerr << "[get_buf_len] Cannot set IMMEDIATE mode of bpf device"
              << std::endl;
    return -1;
  }

  // request buffer length
  if (ioctl(bpf, BIOCGBLEN, &buf_len) == -1) {
    std::cerr << "[get_buf_len] Cannot get buffer length of bpf device"
              << std::endl;
    return -1;
  }
  std::cerr << "[get_buf_len] Buffer length of bpf device: " << buf_len
            << std::endl;
  return buf_len;
}

// Divide data across net frames
void write_frames(int bpf, std::span<uint8_t> data) {
  size_t start = 0;

  size_t bytes_to_send;
  ssize_t bytes_sent;

  while (!data.empty()) {
    if (data.size() < net::MAX_PAYLOAD_LEN)
      bytes_to_send = data.size();
    else
      bytes_to_send = net::MAX_PAYLOAD_LEN;

    // Fill frame payload
    std::cerr << "Copying payload from " << start << ", length" << bytes_to_send
              << std::endl;
    net::frame frame(dest_mac, src_mac,
                     std::span<uint8_t>(data.data(), bytes_to_send));

    // Note we don't add the four-byte CRC, the OS does this for us.
    // Neither do we fill packets with zeroes when the frame length is
    // below the minimum Ethernet frame length, the OS will do the
    // padding.
    std::cerr << "Total frame length: " << frame.size()
              << " of maximum net frame length" << ETHER_MAX_LEN - ETHER_CRC_LEN
              << std::endl;

    auto bytes = frame.to_bytes();
    bytes_sent = ::write(bpf, bytes.data(), bytes.size());
    // Check results
    if (bytes_sent < 0) {
      perror("Error, perhaps device doesn't have IP address assigned?");
      exit(1);
    } else if (bytes_sent != frame.size()) {
      std::cerr << "Error, only sent " << bytes_sent << " bytes of "
                << frame.size() << std::endl;
    } else {
      std::cerr << "Sending frame OK" << std::endl;
    }

    std::cerr << "size of data = " << data.size() << std::endl;
    data = data.subspan(frame.payload_size());
    std::cerr << "size of data = " << data.size() << std::endl;
  }
}

// Create a simple ramp so we can check the splitting of data across frames on
// the other side (using tcpdump or somesuch)
std::vector<uint8_t> make_testdata(int len) {
  std::vector<uint8_t> buf(len);
  std::iota(buf.begin(), buf.end(), 0);
  return buf;
}

int main() {
  std::string interface("en0");
  auto bpf = socket_guard{open_dev()};
  if (bpf == invalid_fd) {
    std::cerr << "[main] failed to open bpf" << std::endl;
    return -1;
  }
  if (!assoc_dev(*bpf, interface))
    return -1;
  auto buf_len = get_buf_len(*bpf);
  if (buf_len == -1) {
    std::cerr << "[main] failed to get buffer length" << std::endl;
    return -1;
  }

  auto testdata = make_testdata(100);
  write_frames(*bpf, testdata);

  // read_single_frame(bpf, buf_len);
  // read_frames(bpf, buf_len);

  write_frames(bpf, testdata, testdata_len);
  return 0;
}

#endif
 *
 * */
