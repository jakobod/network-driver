/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/pipe_socket.hpp"

#include "util/error_or.hpp"
#include "util/logger.hpp"

#include <unistd.h>

namespace net {

util::error_or<pipe_socket_pair> make_pipe() {
  std::array<socket_id, 2> pipefds;
  if (pipe(pipefds.data()) != 0)
    return util::error(util::error_code::socket_operation_failed,
                       "make_pipe: {0}", last_socket_error_as_string());
  LOG_DEBUG("Created pipe with fds=[", pipefds[0], ",", pipefds[1], "]");
  return std::make_pair(pipe_socket{pipefds[0]}, pipe_socket{pipefds[1]});
}

ptrdiff_t write(pipe_socket x, util::const_byte_span buf) {
  LOG_DEBUG("Writing ", buf.size(), " bytes on pipe with ",
            NET_ARG2("fd", x.id));
  return ::write(x.id, reinterpret_cast<const char*>(buf.data()), buf.size());
}

ptrdiff_t read(pipe_socket x, util::byte_span buf) {
  LOG_DEBUG("Reading ", buf.size(), " bytes from pipe with ",
            NET_ARG2("fd", x.id));
  return ::read(x.id, reinterpret_cast<char*>(buf.data()), buf.size());
}

} // namespace net
