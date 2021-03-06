/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/stream_socket.hpp"

#include "util/error_or.hpp"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <utility>

namespace net {

constexpr int no_sigpipe_io_flag = MSG_NOSIGNAL;

util::error_or<stream_socket_pair> make_stream_socket_pair() {
  socket_id sockets[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0)
    return util::error(util::error_code::socket_operation_failed,
                       last_socket_error_as_string());
  return std::make_pair(stream_socket{sockets[0]}, stream_socket{sockets[1]});
}

bool keepalive(stream_socket x, bool new_value) {
  int value = new_value ? 1 : 0;
  auto res = setsockopt(x.id, SOL_SOCKET, SO_KEEPALIVE, &value,
                        static_cast<unsigned>(sizeof(value)));
  return res == 0;
}

bool nodelay(stream_socket x, bool new_value) {
  int flag = new_value ? 1 : 0;
  auto res = setsockopt(x.id, IPPROTO_TCP, TCP_NODELAY,
                        reinterpret_cast<const void*>(&flag),
                        static_cast<int>(sizeof(flag)));
  return res == 0;
}

ptrdiff_t read(stream_socket x, util::byte_span buf) {
  return ::recv(x.id, reinterpret_cast<void*>(buf.data()), buf.size(),
                no_sigpipe_io_flag);
}

ptrdiff_t write(stream_socket x, util::const_byte_span buf) {
  return ::send(x.id, reinterpret_cast<const void*>(buf.data()), buf.size(),
                no_sigpipe_io_flag);
}

} // namespace net
