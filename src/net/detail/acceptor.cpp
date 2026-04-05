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

#if defined(LIB_NET_URING)
#  include "net/detail/uring_manager.hpp"
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

// -- Explicit template instantiations -----------------------------------------

/// @brief Instantiate acceptor_base for event_handler (epoll/kqueue)
template class acceptor_base<event_handler>;

#if defined(LIB_NET_URING)
/// @brief Instantiate acceptor_base for uring_manager (io_uring)
template class acceptor_base<uring_manager>;
#endif

} // namespace net::detail
