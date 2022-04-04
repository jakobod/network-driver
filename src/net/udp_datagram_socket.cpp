/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/udp_datagram_socket.hpp"

#include "net/socket_guard.hpp"
#include "net/socket_sys_includes.hpp"

#include "util/error.hpp"

namespace net {

constexpr const int max_conn_backlog = 10;
constexpr const int no_sigpipe_io_flag = MSG_NOSIGNAL;

util::error_or<udp_datagram_socket_result>
make_udp_datagram_socket(std::uint16_t port) {
  udp_datagram_socket sock{::socket(AF_INET, SOCK_DGRAM, 0)};
  auto guard = make_socket_guard(sock);
  if (sock == invalid_socket)
    return util::error(util::error_code::socket_operation_failed,
                       "Failed to create socket");
  if (auto err = bind(sock, ip::v4_endpoint{htonl(INADDR_ANY), htons(port)}))
    return err;
  auto res = net::port_of(*guard);
  if (auto err = get_error(res))
    return *err;
  return std::make_pair(guard.release(), std::get<uint16_t>(res));
}

ptrdiff_t write(udp_datagram_socket x, ip::v4_endpoint ep,
                util::const_byte_span buf) {
  auto dest = ip::to_sockaddr_in(ep);
  return sendto(x.id, reinterpret_cast<const void*>(buf.data()), buf.size(),
                no_sigpipe_io_flag, reinterpret_cast<sockaddr*>(&dest),
                sizeof(sockaddr_in));
}

std::pair<ip::v4_endpoint, ptrdiff_t> read(udp_datagram_socket x,
                                           util::byte_span buf) {
  sockaddr_in source_addr;
  socklen_t size;
  auto res = recvfrom(x.id, reinterpret_cast<void*>(buf.data()), buf.size(),
                      no_sigpipe_io_flag,
                      reinterpret_cast<sockaddr*>(&source_addr), &size);
  return std::make_pair(ip::v4_endpoint{source_addr}, res);
}

} // namespace net
