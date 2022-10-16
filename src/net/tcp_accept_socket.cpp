/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/tcp_accept_socket.hpp"

#include "net/ip/v4_address.hpp"
#include "net/ip/v4_endpoint.hpp"

#include "net/socket_guard.hpp"
#include "net/socket_sys_includes.hpp"
#include "net/tcp_stream_socket.hpp"

#include "util/error.hpp"
#include "util/error_or.hpp"

#include <utility>

namespace net {

util::error listen(tcp_accept_socket sock, int conn_backlog) {
  if (::listen(sock.id, conn_backlog) != 0)
    return util::error{util::error_code::socket_operation_failed,
                       "failed to set socket {0} to listen", sock.id};
  return util::none;
}

tcp_stream_socket accept(tcp_accept_socket sock) {
  sockaddr_in cli = {};
  socklen_t len = sizeof(sockaddr_in);
  return tcp_stream_socket{
    ::accept(sock.id, reinterpret_cast<sockaddr*>(&cli), &len)};
}

util::error_or<acceptor_pair> make_tcp_accept_socket(const ip::v4_endpoint& ep,
                                                     const int conn_backlog) {
  const tcp_accept_socket sock{::socket(AF_INET, SOCK_STREAM, 0)};
  if (sock == invalid_socket) {
    return util::error(util::error_code::socket_operation_failed,
                       "Failed to create socket {0}",
                       util::last_error_as_string());
  }
  auto guard = make_socket_guard(sock);
  if (auto err = bind(sock, ep))
    return err;
  if (auto err = listen(sock, conn_backlog))
    return err;
  auto res = port_of(*guard);
  if (auto err = util::get_error(res))
    return *err;
  return std::make_pair(guard.release(), std::get<uint16_t>(res));
}

} // namespace net
