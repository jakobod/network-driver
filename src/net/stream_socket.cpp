/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 31.03.2021
 *
 *  This file is based on `stream_socket.cpp` from the C++ Actor Framework.
 *  https://github.com/actor-framework/incubator
 */

#include "net/stream_socket.hpp"

#include <netinet/in.h>
#include <netinet/tcp.h>

namespace net {

constexpr int no_sigpipe_io_flag = MSG_NOSIGNAL;

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

ptrdiff_t read(stream_socket x, detail::byte_span buf) {
  return ::recv(x.id, reinterpret_cast<char*>(buf.data()), buf.size(),
                no_sigpipe_io_flag);
}

ptrdiff_t write(stream_socket x, detail::byte_span buf) {
  return ::send(x.id, reinterpret_cast<const char*>(buf.data()), buf.size(),
                no_sigpipe_io_flag);
}

} // namespace net