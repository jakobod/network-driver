/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 02.03.2021
 */

#include "net/raw_socket.hpp"

#include "detail/socket_guard.hpp"
#include "net/socket_sys_includes.hpp"

namespace net {

raw_socket make_raw_socket() {
  auto raw_sock = raw_socket{::socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)};
  if (raw_sock == invalid_socket) {
    perror("socket");
    close(raw_sock);
    abort();
  }
  return raw_sock;
}

ssize_t sendto(raw_socket sock, sockaddr_ll socket_address,
               detail::byte_span data) {
  return ::sendto(sock.id, data.data(), data.size(), 0,
                  (sockaddr*) &socket_address, sizeof(sockaddr_ll));
}

} // namespace net
