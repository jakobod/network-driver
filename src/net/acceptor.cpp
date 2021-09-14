/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
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
  auto hdl = handle<tcp_accept_socket>();
  auto accepted = accept(hdl);
  if (accepted == invalid_socket) {
    mpx()->handle_error(
      error(socket_operation_failed,
            "accepting failed: " + last_socket_error_as_string()));
    return true;
  }
  if (!nonblocking(accepted, true)) {
    mpx()->handle_error(
      error(socket_operation_failed,
            "nonblocking failed " + last_socket_error_as_string()));
    return true;
  }
  auto mgr = factory_->make(accepted, mpx());
  mpx()->add(std::move(mgr), operation::read);
  return true;
}

bool acceptor::handle_write_event() {
  mpx()->handle_error(
    error(runtime_error, "[acceptor::handle_write_event()] acceptor should not "
                         "be registered for writing"));
  return false;
}

bool acceptor::handle_timeout(uint64_t) {
  mpx()->handle_error(
    error(runtime_error, "[acceptor::handle_timeout()] not implemented!"));
  return false;
}

} // namespace net
