/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/ip/fwd.hpp"

#include <cstddef>
#include <memory>
#include <utility>

// -- net namespace forward declarations ---------------------------------------

namespace net {

// -- classes ------------------------------------------------------------------

class acceptor;
class multiplexer;
class multiplexer_impl;
class pollset_updater;
class socket_manager;
class socket_manager_factory;

// -- structs ------------------------------------------------------------------

struct application;
struct datagram_socket;
struct layer;
struct pipe_socket;
struct raw_socket;
struct receive_policy;
struct socket;
struct stream_socket;
struct tcp_stream_socket;
struct tcp_accept_socket;
struct timeout_entry;

// -- enums --------------------------------------------------------------------

enum class event_result : uint8_t;
enum class operation : uint32_t;

// -- pair types ---------------------------------------------------------------

using acceptor_pair = std::pair<tcp_accept_socket, uint16_t>;
using datagram_socket_pair = std::pair<datagram_socket, datagram_socket>;

// -- pointer types ------------------------------------------------------------

using multiplexer_ptr = std::shared_ptr<multiplexer>;
using socket_manager_ptr = std::shared_ptr<socket_manager>;
using socket_manager_factory_ptr = std::shared_ptr<socket_manager_factory>;

// -- template types -----------------------------------------------------------

template <class Socket>
class socket_guard;
template <class NextLayer>
class transport_adaptor;

} // namespace net
