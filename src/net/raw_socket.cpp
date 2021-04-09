/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 02.03.2021
 */

#include "net/raw_socket.hpp"

#include <sys/socket.h>

#include "detail/error.hpp"
#include "detail/socket_guard.hpp"
#include "net/socket_sys_includes.hpp"

namespace net {

detail::error_or<raw_socket> make_raw_socket() {
  auto raw_sock = raw_socket{::socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)};
  auto guard = detail::make_socket_guard(raw_sock);
  if (guard == invalid_socket)
    return detail::error(detail::socket_operation_failed,
                         last_socket_error_as_string());
  return guard.release();
}

ssize_t sendto(raw_socket sock, sockaddr_ll socket_address,
               detail::byte_span data) {
  return ::sendto(sock.id, data.data(), data.size(), 0,
                  (sockaddr*) &socket_address, sizeof(sockaddr_ll));
}

} // namespace net