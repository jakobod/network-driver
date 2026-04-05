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

/// @brief Forward declaration of URI parser class.
class uri;

// -- structs ------------------------------------------------------------------

/// @brief Forward declaration of application protocol interface.
struct application;

/// @brief Forward declaration of datagram socket class.
struct datagram_socket;

/// @brief Forward declaration of protocol layer interface.
struct layer;

/// @brief Forward declaration of IPC pipe socket class.
struct pipe_socket;

/// @brief Forward declaration of raw socket class.
struct raw_socket;

/// @brief Forward declaration of receive buffer policy class.
struct receive_policy;

/// @brief Forward declaration of base socket class.
struct socket;

/// @brief Forward declaration of TCP stream socket class.
struct stream_socket;

/// @brief Forward declaration of TCP acceptor socket class.
struct tcp_accept_socket;

/// @brief Forward declaration of TCP stream socket class.
struct tcp_stream_socket;

/// @brief Forward declaration of timeout scheduling entry.
struct timeout_entry;

// -- enums --------------------------------------------------------------------

/// @brief Forward declaration of event result enumeration.
enum class event_result : std::uint8_t;

/// @brief Forward declaration of socket operation enumeration.
enum class operation : std::uint8_t;

// -- type aliases -------------------------------------------------------------

/// @brief Pair type for acceptor socket and listening port.
using acceptor_pair = std::pair<tcp_accept_socket, std::uint16_t>;

/// @brief Pair type for two datagram sockets (e.g., bidirectional pipe).
using datagram_socket_pair = std::pair<datagram_socket, datagram_socket>;

/// @brief Pair type for two IPC pipe sockets.
using pipe_socket_pair = std::pair<pipe_socket, pipe_socket>;

// -- template types ----------------------------------------------------------

/// @brief Forward declaration of RAII socket guard template.
/// @tparam Socket A type derived from socket.
template <meta::derived_from<socket> Socket>
class socket_guard;

/// @brief Forward declaration of transport-to-layer adaptor template.
/// @tparam NextLayer The next protocol layer in the stack.
template <class NextLayer>
class transport_adaptor;

} // namespace net

namespace net::detail {
/// @brief Forward declaration of base socket manager class.
class manager_base;

/// @brief Forward declaration of event multiplexer base class.
class multiplexer_base;

/// @brief Forward declaration of pollset updater template.
/// @tparam Base The base manager type.
template <class Base>
class pollset_updater;

/// @brief Forward declaration of connection acceptor template.
/// @tparam Base The base manager type.
template <class Base>
class acceptor;
} // namespace net::detail
