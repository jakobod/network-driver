/**
 *  @author    Jakob Otto
 *  @file      socket.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/socket/socket.hpp"

#include "net/ip/v4_endpoint.hpp"

#include "util/error.hpp"
#include "util/logger.hpp"

#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace net {

void close(socket hdl) {
  LOG_DEBUG("closing ", NET_ARG2("socket", hdl.id));
  if (hdl != invalid_socket)
    ::close(hdl.id);
}

void shutdown(socket hdl, int how) {
  LOG_DEBUG("shutdown ", NET_ARG2("socket", hdl.id), ", ", NET_ARG(how));
  if (hdl != invalid_socket)
    ::shutdown(hdl.id, how);
}

util::error bind(socket hdl, ip::v4_endpoint ep) {
  LOG_DEBUG("binding ", NET_ARG2("socket", hdl.id), " to ",
            NET_ARG2("endpoint", to_string(ep)));
  const auto sin = ip::to_sockaddr_in(ep);
  if (::bind(hdl.id, reinterpret_cast<const sockaddr*>(&sin),
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
  LOG_DEBUG("nonblocking on ", NET_ARG2("socket", hdl.id), ", ",
            NET_ARG(new_value));
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

bool reuseaddr(socket sock, bool new_value) {
  LOG_DEBUG("reuseaddr on ", NET_ARG2("socket", sock.id), ", ",
            NET_ARG(new_value));
  int on = new_value ? 1 : 0;
  const auto res = setsockopt(sock.id, SOL_SOCKET, SO_REUSEADDR,
                              reinterpret_cast<const void*>(&on),
                              static_cast<unsigned>(sizeof(on)));
  return res == 0;
}

} // namespace net
