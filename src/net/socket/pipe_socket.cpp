/**
 *  @author    Jakob Otto
 *  @file      pipe_socket.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/socket/pipe_socket.hpp"

#include "util/error_or.hpp"
#include "util/logger.hpp"

#include <algorithm>
#include <sys/uio.h>
#include <unistd.h>

namespace net {

util::error_or<pipe_socket_pair> make_pipe() {
  std::array<socket_id, 2> pipefds;
  if (pipe(pipefds.data()) != 0) {
    return util::error(util::error_code::socket_operation_failed,
                       "make_pipe: {0}", last_socket_error_as_string());
  }
  LOG_DEBUG("Created pipe with fds=[", pipefds[0], ",", pipefds[1], "]");
  return std::make_pair(pipe_socket{pipefds[0]}, pipe_socket{pipefds[1]});
}

ptrdiff_t read(pipe_socket x, util::byte_span buf) {
  LOG_DEBUG("Reading ", buf.size(), " bytes from pipe with ",
            NET_ARG2("fd", x.id));
  return ::read(x.id, reinterpret_cast<char*>(buf.data()), buf.size());
}

ptrdiff_t write(pipe_socket x, util::const_byte_span buf) {
  LOG_DEBUG("Writing ", buf.size(), " bytes on pipe with ",
            NET_ARG2("fd", x.id));
  return ::write(x.id, reinterpret_cast<const char*>(buf.data()), buf.size());
}

std::ptrdiff_t readv(pipe_socket x, std::span<iovec> iovecs) {
  // TODO calculate number of bytes
  LOG_DEBUG("Reading ", " bytes on pipe with ", NET_ARG2("fd", x.id));
  return ::readv(x.id, iovecs.data(), iovecs.size());
}

/// @brief Writes data to a pipe socket (sending side).
/// Transmits the contents of the buffer to the connected peer.
/// @param x The pipe socket to write to (must be the write end of a pipe).
/// @param buf The data to transmit.
/// @return The number of bytes written, or a negative value on error.
[[nodiscard]] std::ptrdiff_t writev(pipe_socket x, std::span<iovec> iovecs) {
  // TODO calculate number of bytes
  LOG_DEBUG("Writing ", " bytes on pipe with ", NET_ARG2("fd", x.id));
  return ::writev(x.id, iovecs.data(), iovecs.size());
}

} // namespace net
