/**
 *  @author    Jakob Otto
 *  @file      acceptor.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "net/event_result.hpp"

#include "net/detail/multiplexer_base.hpp"

#include "net/socket/socket.hpp"
#include "net/socket/tcp_accept_socket.hpp"
#include "net/socket/tcp_stream_socket.hpp"

#include "util/logger.hpp"

#include <functional>

namespace net::detail {

/// Manages the lifetime of a socket.
template <class Base>
class acceptor : public Base {
  using manager_factory = std::function<manager_base_ptr(net::socket)>;

public:
  acceptor(tcp_accept_socket handle, multiplexer_base* mpx,
           manager_factory factory)
    : Base{handle, mpx}, factory_{std::move(factory)} {
    LOG_TRACE();
  }

  // -- event handling ---------------------------------------------------------

  virtual event_result handle_read_event() {
    auto hdl = Base::template handle<tcp_accept_socket>();
    LOG_TRACE();
    LOG_DEBUG("event_acceptor handling read event ",
              NET_ARG2("handle", hdl.id));
    auto accepted = accept(hdl);
    if (accepted == invalid_socket) {
      return event_result::ok;
    }
    LOG_DEBUG("event_acceptor connection ",
              NET_ARG2("new_handle", accepted.id));
    return handle_accepted(accepted);
  }

  virtual event_result handle_completion(operation op, int res) {
    if (op != operation::accept) {
      LOG_ERROR("acceptor called for operation != accept");
      return event_result::error;
    }
    LOG_DEBUG("acceptor handling accepted connection on ",
              NET_ARG2("handle", Base::handle().id),
              NET_ARG2("new_handle", res));

    if (res <= 0) {
      LOG_ERROR("error with accepted connection");
      return event_result::ok;
    }
    return handle_accepted(tcp_stream_socket{res});
  }

private:
  event_result handle_accepted(tcp_stream_socket accepted) {
    if (!nonblocking(accepted, true)) {
      return event_result::ok;
    }
    auto mgr = factory_(accepted);
    Base::mpx()->add(std::move(mgr), operation::read);
    return event_result::ok;
  }

  manager_factory factory_;
};

} // namespace net::detail
