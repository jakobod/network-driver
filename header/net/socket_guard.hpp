/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 18.02.2021
 */

#pragma once

#include <iostream>

#include "net/socket.hpp"

static constexpr int invalid_fd = -1;

namespace net {

template <class Socket>
class socket_guard {
public:
  static_assert(std::is_base_of_v<socket, Socket>,
                "Template argument is NOT a socket!");

  explicit socket_guard(Socket sock) : sock_(sock) {
    // nop
  }

  socket_guard() : socket_guard(-1) {
    // nop
  }

  ~socket_guard() {
    if (sock_ != invalid_socket) {
      close(sock_);
    }
  }

  Socket release() {
    auto ret = sock_;
    sock_.id = invalid_fd;
    return ret;
  }

  Socket operator*() {
    return sock_;
  }

  Socket operator->() {
    return sock_;
  }

  template <class Other>
  bool operator==(const Other& other) {
    return sock_ == other;
  }

private:
  Socket sock_;
};

template <class Socket>
socket_guard<Socket> make_socket_guard(Socket sock) {
  return socket_guard<Socket>{sock};
}

} // namespace net
