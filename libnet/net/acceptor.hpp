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
#include "net/multiplexer_base.hpp"

#include <functional>

namespace net {

/// Manages the lifetime of a socket.
class acceptor : public manager_base {
public:
  using factory_type
    = std::function<manager_base_ptr(net::socket, multiplexer_base*)>;

  acceptor(tcp_accept_socket handle, multiplexer_base* mpx,
           factory_type factory);

  virtual ~acceptor() = default;

  // -- properties -------------------------------------------------------------

  event_result handle_read_event() override;

private:
  factory_type factory_;
};

} // namespace net
