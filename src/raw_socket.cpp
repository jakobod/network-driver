/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 02.03.2021
 */

#include "net/raw_socket.hpp"

#include <netinet/in.h>
#include <sys/socket.h>

#include "detail/socket_guard.hpp"

namespace net {

raw_socket make_raw_socket() {
  auto guard
    = detail::make_socket_guard(::socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW));
  if (guard == invalid_socket_id) {
    perror("socket");
    abort();
  }
  return raw_socket{guard.release()};
}

ssize_t sendto(raw_socket sock, sockaddr_ll socket_address,
               detail::byte_span data) {
  return ::sendto(sock.id, data.data(), data.size(), 0,
                  (sockaddr*) &socket_address, sizeof(sockaddr_ll));
}

} // namespace net
