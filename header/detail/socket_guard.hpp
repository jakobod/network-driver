/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 18.02.2021
 */

#pragma once

#include <iostream>
#include <unistd.h>

static constexpr int invalid_fd = -1;

namespace detail {

template <class Socket>
class socket_guard {
public:
  socket_guard(Socket sock) : sock_(sock) {
    // nop
  }

  ~socket_guard() {
    if (sock_ != invalid_fd) {
      ::close(sock_);
    }
  }

  int release() {
    auto sock = sock_;
    sock_ = invalid_fd;
    return sock;
  }

  int operator*() {
    return sock_;
  }

  bool operator==(Socket other) {
    return sock_ == other;
  }

private:
  Socket sock_;
};

template <class Socket>
socket_guard<Socket> make_socket_guard(Socket sock) {
  return {sock};
}

} // namespace detail
