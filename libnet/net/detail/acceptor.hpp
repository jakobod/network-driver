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
#include "net/detail/multiplexer_base.hpp"

#include "net/socket/socket.hpp"

#include <functional>

namespace net::detail {

/// Manages the lifetime of a socket.
class acceptor : public event_handler {
  using manager_factory = std::function<manager_base_ptr(net::socket)>;

public:
  acceptor(tcp_accept_socket handle, multiplexer_base* mpx,
           manager_factory factory);

  virtual ~acceptor() = default;

  acceptor(const acceptor& other) = default;
  acceptor(acceptor&& other) noexcept = default;
  acceptor& operator=(const acceptor& other) = default;
  acceptor& operator=(acceptor&& other) noexcept = default;

  // -- event handling ---------------------------------------------------------

  event_result handle_read_event() override;

private:
  manager_factory factory_;
};

} // namespace net::detail
