/**
 *  @author    Jakob Otto
 *  @file      acceptor.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/acceptor.hpp"

#include "net/event_result.hpp"
#include "net/multiplexer_base.hpp"

#include "net/socket/tcp_accept_socket.hpp"
#include "net/socket/tcp_stream_socket.hpp"

#include "util/error.hpp"
#include "util/logger.hpp"

namespace net {

acceptor::acceptor(tcp_accept_socket handle, multiplexer_base* mpx,
                   manager_factory factory)
  : manager_base(handle, mpx), factory_(std::move(factory)) {
  LOG_TRACE();
}

event_result acceptor::handle_read_event() {
  auto hdl = handle<tcp_accept_socket>();
  LOG_TRACE();
  LOG_DEBUG("acceptor handling read event ", NET_ARG2("handle", hdl.id));
  auto accepted = accept(hdl);
  if (accepted == invalid_socket) {
    mpx()->handle_error({util::error_code::socket_operation_failed,
                         "[acceptor::handle_read_event()] accept failed: {0}",
                         last_socket_error_as_string()});
    return event_result::ok;
  }
  LOG_DEBUG("accepted connection ", NET_ARG2("new_handle", accepted.id));
  if (!nonblocking(accepted, true)) {
    mpx()->handle_error(
      {util::error_code::socket_operation_failed,
       "[acceptor::handle_read_event()] nonblocking failed: {0}",
       last_socket_error_as_string()});
    return event_result::ok;
  }
  auto mgr = factory_(accepted, mpx());
  mpx()->add(std::move(mgr), operation::read);
  return event_result::ok;
}

} // namespace net
