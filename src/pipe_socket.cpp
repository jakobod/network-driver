/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 04.03.2021
 *
 *  This file is based on `pipe_socket.cpp` from the C++ Actor Framework.
 *  https://github.com/actor-framework/incubator
 */

#include "net/pipe_socket.hpp"

#include <iostream>
#include <unistd.h>

namespace net {

pipe_socket_pair make_pipe() {
  socket_id pipefds[2];
  if (pipe(pipefds) != 0) {
    std::cerr << "make_pipe failed: " << last_socket_error_as_string()
              << std::endl;
    abort();
  }
  return std::make_pair(pipe_socket{pipefds[0]}, pipe_socket{pipefds[1]});
}

ptrdiff_t write(pipe_socket x, detail::byte_span buf) {
  return ::write(x.id, reinterpret_cast<const char*>(buf.data()), buf.size());
}

ptrdiff_t read(pipe_socket x, detail::byte_span buf) {
  return ::read(x.id, reinterpret_cast<char*>(buf.data()), buf.size());
}

} // namespace net
