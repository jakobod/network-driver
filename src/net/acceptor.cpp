/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 30.03.2021
 */

#include "net/acceptor.hpp"

#include <memory>

#include "net/multiplexer.hpp"
#include "net/socket_manager_impl.hpp"
#include "net/tcp_accept_socket.hpp"
#include "net/tcp_stream_socket.hpp"

namespace net {

acceptor::acceptor(tcp_accept_socket handle, multiplexer* mpx)
  : socket_manager(handle, mpx) {
  // nop
}

// -- properties -------------------------------------------------------------

bool acceptor::handle_read_event() {
  auto accept_socket = socket_cast<tcp_accept_socket>(handle());
  auto accepted = accept(accept_socket);
  if (accepted == invalid_socket) {
    std::cerr << "accepting failed!" << std::endl;
    return false;
  }
  auto mgr = std::make_shared<socket_manager_impl>(accepted, mpx());
  mpx()->register_new_socket_manager(std::move(mgr));
  return true;
}

bool acceptor::handle_write_event() {
  std::cerr << "acceptor should NOT receive write events!" << std::endl;
  return false;
}

} // namespace net
