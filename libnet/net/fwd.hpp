/**
 *  @author    Jakob Otto
 *  @file      fwd.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/ip/fwd.hpp"
#include "util/fwd.hpp"

#include "util/intrusive_ptr.hpp"

#include "meta/concepts.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace net {

// -- classes ------------------------------------------------------------------

class manager_factory;
class multiplexer;
class pollset_updater;
class uri;
class uring_multiplexer;

// -- structs ------------------------------------------------------------------

struct application;
struct datagram_socket;
struct layer;
struct pipe_socket;
struct raw_socket;
struct receive_policy;
struct socket;
struct stream_socket;
struct tcp_accept_socket;
struct tcp_stream_socket;
struct timeout_entry;
struct udp_datagram_socket;

// -- enums --------------------------------------------------------------------

enum class event_result : std::uint8_t;
enum class operation : std::uint32_t;

// -- type aliases -------------------------------------------------------------

using acceptor_pair        = std::pair<tcp_accept_socket, std::uint16_t>;
using application_ptr      = std::unique_ptr<application>;
using datagram_socket_pair = std::pair<datagram_socket, datagram_socket>;
using manager_factory_ptr  = std::unique_ptr<manager_factory>;
using multiplexer_ptr      = std::shared_ptr<multiplexer>;
using pipe_socket_pair     = std::pair<pipe_socket, pipe_socket>;

// -- template types -----------------------------------------------------------

template <meta::derived_from<socket> Socket>
class socket_guard;
template <class NextLayer>
class transport_adaptor;

} // namespace net

namespace net::ip {

class v4_address;
class v4_endpoint;

} // namespace net::ip

namespace net::sockets {

struct datagram_socket;
struct pipe_socket;
struct socket;
struct stream_socket;
struct tcp_accept_socket;
struct tcp_stream_socket;
struct udp_datagram_socket;

} // namespace net::sockets

namespace net::uring {

class manager;
class multiplexer_impl;

} // namespace net::uring
