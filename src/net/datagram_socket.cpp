/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/datagram_socket.hpp"

#include "util/error.hpp"

#include <netinet/in.h>
#include <netinet/udp.h>
#include <utility>

namespace net {

constexpr int no_sigpipe_io_flag = MSG_NOSIGNAL;

util::error_or<datagram_socket_pair> make_connected_datagram_socket_pair() {
  std::array<socket_id, 2> sockets;
  if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sockets.data()) < 0)
    return util::error(util::error_code::socket_operation_failed,
                       last_socket_error_as_string());
  return std::make_pair(datagram_socket{sockets[0]},
                        datagram_socket{sockets[1]});
}

ptrdiff_t read(datagram_socket x, util::byte_span buf) {
  return recv(x.id, reinterpret_cast<char*>(buf.data()), buf.size(),
              no_sigpipe_io_flag);
}

ptrdiff_t write(datagram_socket x, util::const_byte_span buf) {
  return send(x.id, reinterpret_cast<const char*>(buf.data()), buf.size(),
              no_sigpipe_io_flag);
}

} // namespace net
