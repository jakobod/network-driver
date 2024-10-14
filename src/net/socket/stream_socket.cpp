/**
 *  @author    Jakob Otto
 *  @file      stream_socket.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/socket/stream_socket.hpp"

#include "net/socket/socket.hpp"
#include "net/socket_id.hpp"

#include "util/error.hpp"
#include "util/logger.hpp"

#include "util/fwd.hpp"

#include <array>
#include <cstddef>
#include <utility>

#include <sys/socket.h>

namespace {

constexpr int no_sigpipe_io_flag = MSG_NOSIGNAL;

}

namespace net {

util::error_or<stream_socket_pair> make_stream_socket_pair() {
  std::array<socket_id, 2> sockets;
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets.data()) < 0)
    return util::error(util::error_code::socket_operation_failed,
                       last_socket_error_as_string());
  LOG_DEBUG("Created stream_socket_pair with fds=[", sockets[0], ",",
            sockets[1], "]");
  return std::make_pair(stream_socket{sockets[0]}, stream_socket{sockets[1]});
}

bool keepalive(stream_socket hdl, bool new_value) {
  LOG_DEBUG("keepalive on ", NET_ARG2("socket", hdl.id), ", ",
            NET_ARG(new_value));
  int value = new_value ? 1 : 0;
  auto res = setsockopt(hdl.id, SOL_SOCKET, SO_KEEPALIVE, &value,
                        static_cast<unsigned>(sizeof(value)));
  return res == 0;
}

ptrdiff_t read(stream_socket hdl, util::byte_span buf) {
  LOG_DEBUG("Reading ", buf.size(), " bytes from stream_socket with ",
            NET_ARG2("fd", hdl.id));
  return ::recv(hdl.id, reinterpret_cast<void*>(buf.data()), buf.size(),
                no_sigpipe_io_flag);
}

ptrdiff_t write(stream_socket hdl, util::const_byte_span buf) {
  LOG_DEBUG("Writing ", buf.size(), " bytes to stream_socket with ",
            NET_ARG2("fd", hdl.id));
  return ::send(hdl.id, reinterpret_cast<const void*>(buf.data()), buf.size(),
                no_sigpipe_io_flag);
}

} // namespace net
