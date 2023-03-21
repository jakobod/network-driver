/**
 *  @author    Jakob Otto
 *  @file      udp_datagram_socket.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/udp_datagram_socket.hpp"

#include "net/ip/v4_address.hpp"
#include "net/ip/v4_endpoint.hpp"
#include "net/socket_guard.hpp"

#include "util/error.hpp"
#include "util/error_or.hpp"
#include "util/logger.hpp"

#include <netinet/in.h>
#include <utility>

using namespace net::ip;

namespace net {

constexpr const int no_sigpipe_io_flag = MSG_NOSIGNAL;

util::error_or<udp_datagram_socket_result>
make_udp_datagram_socket(std::uint16_t port) {
  const udp_datagram_socket sock{::socket(AF_INET, SOCK_DGRAM, 0)};
  if (sock == invalid_socket)
    return util::error(util::error_code::socket_operation_failed,
                       "Failed to create socket");
  LOG_DEBUG("Created new socket with ", NET_ARG2("id", sock.id));
  auto guard = make_socket_guard(sock);
  if (auto err = bind(sock, v4_endpoint{v4_address::any, htons(port)}))
    return err;
  auto res = net::port_of(*guard);
  if (auto err = util::get_error(res))
    return *err;
  LOG_DEBUG("Bound ", NET_ARG2("socket", sock.id), " to ",
            NET_ARG2("port", std::get<uint16_t>(res)));
  return std::make_pair(guard.release(), std::get<uint16_t>(res));
}

ptrdiff_t write(udp_datagram_socket hdl, v4_endpoint ep,
                util::const_byte_span buf) {
  LOG_DEBUG("Writing ", buf.size(), " bytes to udp_datagram_socket with ",
            NET_ARG2("fd", hdl.id));
  const auto dest = to_sockaddr_in(ep);
  return sendto(hdl.id, reinterpret_cast<const void*>(buf.data()), buf.size(),
                no_sigpipe_io_flag, reinterpret_cast<const sockaddr*>(&dest),
                sizeof(sockaddr_in));
}

udp_read_result read(udp_datagram_socket hdl, util::byte_span buf) {
  LOG_DEBUG("Reading ", buf.size(), " bytes from udp_datagram_socket with ",
            NET_ARG2("fd", hdl.id));
  sockaddr_in source_addr;
  socklen_t size;
  auto res = recvfrom(hdl.id, reinterpret_cast<void*>(buf.data()), buf.size(),
                      no_sigpipe_io_flag,
                      reinterpret_cast<sockaddr*>(&source_addr), &size);
  return std::make_pair(v4_endpoint{source_addr}, res);
}

} // namespace net
