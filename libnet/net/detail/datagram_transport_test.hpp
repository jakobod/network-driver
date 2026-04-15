/**
 *  @author    Jakob Otto
 *  @file      datagram_transport.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "net/detail/event_handler.hpp"
#include "net/detail/multiplexer_base.hpp"
#include "net/detail/transport_base.hpp"
#if defined(LIB_NET_URING)
#  include "net/detail/uring_manager.hpp"
#  include "net/detail/uring_multiplexer.hpp"
#endif

#include "net/manager_result.hpp"
#include "net/receive_policy.hpp"

#include "net/socket/datagram_socket.hpp"
#include "net/socket/udp_datagram_socket.hpp"

#include "net/ip/v4_endpoint.hpp"

#include "util/assert.hpp"
#include "util/config.hpp"
#include "util/error.hpp"
#include "util/format.hpp"
#include "util/logger.hpp"

#include <algorithm>
#include <ranges>
#include <sys/uio.h>
#include <utility>

namespace net::detail {

struct datagram {
  datagram(util::byte_buffer buf, ip::v4_endpoint ep)
    : buf_{std::move(buf)}, ep_{std::move(ep)} {
    // nop
  }

  util::byte_buffer buf_;
  ip::v4_endpoint ep_;
  std::uint64_t id_{0};
  bool currently_enqueued_{false};
};

// -- datagram_transport implementation ----------------------------------------

/// @brief Layered datagram (UDP) transport implementation.
/// Provides a transport layer for datagram-based protocols (UDP) with
/// buffer management for reading and writing. Supports layering other
/// protocol handlers on top through the NextLayer template parameter.
/// @tparam NextLayer The upper protocol layer to stack on this transport.
template <class ManagerBase, class NextLayer>
class datagram_transport : public transport_base,
                           public datagram_transport_base,
                           public ManagerBase {
public:
  datagram_transport() = default;
  ~datagram_transport() = default;

private:
};

template <class ManagerBase, class NextLayer>
class datagram_transport_impl;

/// @brief Specialization for event_handler (epoll/kqueue).
template <class NextLayer>
class datagram_transport_impl<event_handler, NextLayer>
  : public datagram_transport<event_handler, NextLayer> {
public:
  datagram_transport_impl() = default;
  ~datagram_transport_impl() = default;

private:
};

template <class NextLayer>
using event_datagram_transport = datagram_transport<event_handler, NextLayer>;

} // namespace net::detail
