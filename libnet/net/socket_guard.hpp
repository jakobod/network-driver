/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/socket.hpp"

namespace net {

template <class Socket>
class socket_guard {
public:
  static_assert(std::is_base_of_v<socket, Socket>,
                "Template argument is NOT a socket!");

  explicit socket_guard(Socket sock) : sock_{sock} {
    // nop
  }

  socket_guard() : socket_guard{invalid_socket} {
    // nop
  }

  ~socket_guard() {
    if (sock_ != invalid_socket)
      close(sock_);
  }

  Socket release() {
    auto ret = sock_;
    sock_.id = invalid_socket_id;
    return ret;
  }

  Socket get() { return sock_; }

  Socket operator*() { return sock_; }

  Socket operator->() { return sock_; }

  bool operator==(const socket_guard<Socket>& other) {
    return sock_ == other.sock_;
  }

private:
  Socket sock_;
};

template <class Socket>
socket_guard<Socket> make_socket_guard(Socket sock) {
  return socket_guard<Socket>{sock};
}

} // namespace net
