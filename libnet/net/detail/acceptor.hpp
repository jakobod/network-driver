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

#include "net/detail/event_handler.hpp"
#if defined(LIB_NET_URING)
#  include "net/detail/uring_manager.hpp"
#endif

#include "net/manager_result.hpp"

#include "net/socket/tcp_accept_socket.hpp"
#include "net/socket/tcp_stream_socket.hpp"

#include "util/logger.hpp"

#include <functional>

namespace net::detail {

using acceptor_factory = std::function<manager_base_ptr(net::socket)>;

/// @brief Generic base for acceptor implementations.
/// Contains shared logic for handling accepted connections.
template <class ManagerBase>
class acceptor_base : public ManagerBase {
public:
  acceptor_base(tcp_accept_socket handle, multiplexer_base* mpx,
                acceptor_factory factory);

protected:
  /// @brief Common handler for accepted connections.
  manager_result handle_accepted(tcp_stream_socket accepted);

private:
  acceptor_factory factory_;
};

template <class ManagerBase>
class acceptor;

/// @brief Specialization for epoll/kqueue-based managers.
/// Handles incoming connections via read events.
template <>
class acceptor<event_handler> : public acceptor_base<event_handler> {
  using base = acceptor_base<event_handler>;

public:
  using base::base;

  /// @brief Handles incoming connection on read event (epoll/kqueue).
  manager_result handle_read_event();
};

using event_handler_acceptor = acceptor<event_handler>;

#if defined(LIB_NET_URING)

template <>
class acceptor<uring_manager> : public acceptor_base<uring_manager> {
  using base = acceptor_base<uring_manager>;

public:
  using base::base;

  operation initial_operation() const noexcept override {
    return operation::accept;
  }

  manager_result enable(operation op) override;

  /// @brief Handles incoming connection on io_uring completion event.
  manager_result handle_completion(operation op, int res,
                                   std::uint64_t id) override;
};

using uring_acceptor = acceptor<uring_manager>;

#endif // LIB_NET_URING

} // namespace net::detail
