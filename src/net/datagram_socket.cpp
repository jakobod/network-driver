/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/datagram_socket.hpp"

#include "util/error.hpp"
#include "util/error_or.hpp"
#include "util/logger.hpp"

#include <sys/socket.h>

namespace net {

constexpr int no_sigpipe_io_flag = MSG_NOSIGNAL;

util::error_or<datagram_socket_pair> make_connected_datagram_socket_pair() {
  LOG_TRACE();
  std::array<socket_id, 2> sockets;
  if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sockets.data()) < 0)
    return util::error(util::error_code::socket_operation_failed,
                       last_socket_error_as_string());
  LOG_DEBUG("Created datagram socket pair with ids=(", sockets.front(), ",",
            sockets.back(), ")");
  return std::make_pair(datagram_socket{sockets[0]},
                        datagram_socket{sockets[1]});
}

std::ptrdiff_t read(datagram_socket x, util::byte_span buf) {
  LOG_TRACE();
  LOG_DEBUG("Reading ", buf.size(), " bytes from datagram socket ", x.id);
  return recv(x.id, reinterpret_cast<char*>(buf.data()), buf.size(),
              no_sigpipe_io_flag);
}

std::ptrdiff_t write(datagram_socket x, util::const_byte_span buf) {
  LOG_TRACE();
  LOG_DEBUG("Writing ", buf.size(), " bytes to datagram socket ", x.id);
  return send(x.id, reinterpret_cast<const char*>(buf.data()), buf.size(),
              no_sigpipe_io_flag);
}

} // namespace net
