/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/socket.hpp"

#include "net/ip/v4_endpoint.hpp"

#include "util/error.hpp"

#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>

namespace net {

void close(socket hdl) {
  if (hdl != invalid_socket)
    ::close(hdl.id);
}

void shutdown(socket hdl, int how) {
  if (hdl != invalid_socket)
    ::shutdown(hdl.id, how);
}

util::error bind(socket sock, ip::v4_endpoint ep) {
  const auto sin = ip::to_sockaddr_in(ep);
  if (::bind(sock.id, reinterpret_cast<const sockaddr*>(&sin),
             sizeof(sockaddr_in))
      != 0)
    return {util::error_code::socket_operation_failed,
            "Failed to bind socket: {0}", last_socket_error_as_string()};
  return util::none;
}

int last_socket_error() {
  return errno;
}

bool last_socket_error_is_temporary() {
  auto code = last_socket_error();
#if EAGAIN == EWOULDBLOCK
  return code == EAGAIN;
#else
  return code == EAGAIN || code == EWOULDBLOCK;
#endif
}

std::string last_socket_error_as_string() {
  return util::last_error_as_string();
}

bool nonblocking(socket hdl, bool new_value) {
  auto flags = fcntl(hdl.id, F_GETFL, 0);
  if (flags == -1)
    return false;
  flags = new_value ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
  return (fcntl(hdl.id, F_SETFL, flags) == 0);
}

util::error_or<uint16_t> port_of(socket x) {
  sockaddr_in sin = {};
  socklen_t len = sizeof(sockaddr_in);
  if (getsockname(x.id, reinterpret_cast<sockaddr*>(&sin), &len) == -1)
    return util::error(util::error_code::socket_operation_failed,
                       last_socket_error_as_string());
  return ntohs(sin.sin_port);
}

bool reuseaddr(socket x, bool new_value) {
  int on = new_value ? 1 : 0;
  auto res = setsockopt(x.id, SOL_SOCKET, SO_REUSEADDR,
                        reinterpret_cast<const void*>(&on),
                        static_cast<unsigned>(sizeof(on)));
  return res == 0;
}

} // namespace net
