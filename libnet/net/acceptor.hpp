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

#include "net/manager_base.hpp"
#include "net/manager_factory.hpp"

namespace net {

/// Manages the lifetime of a socket.
class acceptor : public manager_base {
public:
  acceptor(tcp_accept_socket handle, multiplexer_base* mpx,
           manager_factory factory);

  virtual ~acceptor() = default;

  // -- properties -------------------------------------------------------------

  event_result handle_read_event() override;

private:
  manager_factory factory_;
};

} // namespace net
