/**
 *  @author    Jakob Otto
 *  @file      fwd.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/ip/fwd.hpp"

#include "meta/concepts.hpp"

#include "util/intrusive_ptr.hpp"
#include "util/ref_counted.hpp"

#include <cstddef>
#include <memory>
#include <utility>

// -- net namespace forward declarations ---------------------------------------

namespace net {

// -- classes ------------------------------------------------------------------

class manager_base;
class multiplexer_base;
class pollset_updater;
class uri;
class acceptor;

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

// -- enums --------------------------------------------------------------------

enum class event_result : std::uint8_t;
enum class operation : std::uint32_t;

// -- type aliases -------------------------------------------------------------

using acceptor_pair = std::pair<tcp_accept_socket, std::uint16_t>;
using datagram_socket_pair = std::pair<datagram_socket, datagram_socket>;
using pipe_socket_pair = std::pair<pipe_socket, pipe_socket>;

// -- template types -----------------------------------------------------------

template <meta::derived_from<socket> Socket>
class socket_guard;
template <class NextLayer>
class transport_adaptor;

} // namespace net
