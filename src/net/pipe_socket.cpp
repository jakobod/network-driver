/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *
 *  This file is based on `pipe_socket.cpp` from the C++ Actor Framework.
 *  https://github.com/actor-framework/incubator
 */

#include "net/pipe_socket.hpp"

#include <iostream>
#include <unistd.h>

#include "net/error.hpp"

namespace net {

error_or<pipe_socket_pair> make_pipe() {
  socket_id pipefds[2];
  if (pipe(pipefds) != 0)
    return error(socket_operation_failed,
                 "make_pipe: " + last_socket_error_as_string());
  return std::make_pair(pipe_socket{pipefds[0]}, pipe_socket{pipefds[1]});
}

ptrdiff_t write(pipe_socket x, util::byte_span buf) {
  return ::write(x.id, reinterpret_cast<const char*>(buf.data()), buf.size());
}

ptrdiff_t read(pipe_socket x, util::byte_span buf) {
  return ::read(x.id, reinterpret_cast<char*>(buf.data()), buf.size());
}

} // namespace net
