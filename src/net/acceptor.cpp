/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 30.03.2021
 */

#include "net/acceptor.hpp"

#include <iostream>
#include <memory>

#include "net/multiplexer.hpp"
#include "net/socket_manager_factory.hpp"
#include "net/tcp_accept_socket.hpp"
#include "net/tcp_stream_socket.hpp"

namespace net {

acceptor::acceptor(tcp_accept_socket handle, multiplexer* mpx,
                   socket_manager_factory_ptr factory)
  : socket_manager(handle, mpx), factory_(std::move(factory)) {
  // nop
}

// -- properties -------------------------------------------------------------

bool acceptor::handle_read_event() {
  auto accept_socket = socket_cast<tcp_accept_socket>(handle());
  auto accepted = accept(accept_socket);
  if (!nonblocking(accepted, true))
    mpx()->handle_error(
      detail::error(detail::socket_operation_failed,
                    "nonblocking failed " + last_socket_error_as_string()));
  if (accepted == invalid_socket)
    mpx()->handle_error(
      detail::error(detail::socket_operation_failed,
                    "accepting failed: " + last_socket_error_as_string()));
  auto mgr = factory_->make(accepted, mpx());
  mpx()->add(std::move(mgr), operation::read);
  return true;
}

bool acceptor::handle_write_event() {
  mpx()->handle_error(detail::error(
    detail::runtime_error, "acceptor should NOT receive write events!"));
  return false;
}

} // namespace net
