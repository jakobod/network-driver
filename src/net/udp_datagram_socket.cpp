/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/udp_datagram_socket.hpp"

#include "net/ip/v4_address.hpp"
#include "net/ip/v4_endpoint.hpp"
#include "net/socket_guard.hpp"

#include "util/error.hpp"
#include "util/error_or.hpp"

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
  auto guard = make_socket_guard(sock);
  if (auto err = bind(sock, v4_endpoint{v4_address::any, htons(port)}))
    return err;
  auto res = net::port_of(*guard);
  if (auto err = util::get_error(res))
    return *err;
  return std::make_pair(guard.release(), std::get<uint16_t>(res));
}

ptrdiff_t write(udp_datagram_socket x, v4_endpoint ep,
                util::const_byte_span buf) {
  const auto dest = to_sockaddr_in(ep);
  return sendto(x.id, reinterpret_cast<const void*>(buf.data()), buf.size(),
                no_sigpipe_io_flag, reinterpret_cast<const sockaddr*>(&dest),
                sizeof(sockaddr_in));
}

udp_read_result read(udp_datagram_socket x, util::byte_span buf) {
  sockaddr_in source_addr;
  socklen_t size;
  auto res = recvfrom(x.id, reinterpret_cast<void*>(buf.data()), buf.size(),
                      no_sigpipe_io_flag,
                      reinterpret_cast<sockaddr*>(&source_addr), &size);
  return std::make_pair(v4_endpoint{source_addr}, res);
}

} // namespace net
