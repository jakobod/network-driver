/**
 *  @author    Jakob Otto
 *  @file      acceptor.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/detail/acceptor.hpp"
#include "net/detail/event_handler.hpp"
#include "net/detail/multiplexer_base.hpp"
#include "net/socket/tcp_stream_socket.hpp"

#include "util/error_code.hpp"
#include "util/logger.hpp"

#if defined(LIB_NET_URING)
#  include "net/detail/uring_manager.hpp"
#  include "net/detail/uring_multiplexer.hpp"
#endif

namespace net::detail {

template <class ManagerBase>
acceptor_base<ManagerBase>::acceptor_base(tcp_accept_socket handle,
                                          multiplexer_base* mpx,
                                          acceptor_factory factory)
  : ManagerBase{handle, mpx}, factory_{std::move(factory)} {
  LOG_TRACE();
}

template <class ManagerBase>
event_result
acceptor_base<ManagerBase>::handle_accepted(tcp_stream_socket accepted) {
  auto mgr = factory_(accepted);
  const auto initial = mgr->initial_operation();
  ManagerBase::mpx()->add(std::move(mgr), initial);
  return event_result::ok;
}

// -- acceptor<event_handler> implementation (epoll/kqueue) -------------------

event_result acceptor<event_handler>::handle_read_event() {
  auto accept_handle = handle<tcp_accept_socket>();
  LOG_TRACE();
  LOG_DEBUG("event_acceptor handling read event ",
            NET_ARG2("accept_handle", accept_handle.id));
  const auto accepted = accept(accept_handle);
  if (accepted == invalid_socket) {
    if (net::last_socket_error_is_temporary()) {
      return event_result::ok;
    } else {
      handle_error(util::error{util::error_code::socket_operation_failed,
                               "accept returned an invalid socket: "
                                 + net::last_socket_error_as_string()});
      return event_result::error;
    }
  }
  LOG_DEBUG("event_acceptor connection ", NET_ARG2("new_handle", accepted.id));
  return base::handle_accepted(accepted);
}

#if defined(LIB_NET_URING)

// -- acceptor<uring_manager> implementation (io_uring) -----------------------

operation acceptor<uring_manager>::initial_operation() const noexcept {
  return operation::accept;
}

event_result acceptor<uring_manager>::handle_completion(operation op, int res) {
  if (op != operation::accept) {
    LOG_ERROR("acceptor called for operation != accept");
    return event_result::error;
  }
  LOG_DEBUG("acceptor handling accepted connection on ",
            NET_ARG2("handle", uring_manager::handle().id),
            NET_ARG2("new_handle", res));

  if (res < 0) {
    handle_error(util::error{util::error_code::socket_operation_failed,
                             "accept returned an invalid socket"});
    return event_result::ok;
  }
  const auto handle_res = base::handle_accepted(tcp_stream_socket{res});
  manager_base::mpx<uring_multiplexer>()->submit(this, operation::accept);
  return handle_res;
}

#endif

// -- Explicit template instantiations -----------------------------------------

/// @brief Instantiate acceptor_base for event_handler (epoll/kqueue)
template class acceptor_base<event_handler>;

#if defined(LIB_NET_URING)
/// @brief Instantiate acceptor_base for uring_manager (io_uring)
template class acceptor_base<uring_manager>;
#endif

} // namespace net::detail
