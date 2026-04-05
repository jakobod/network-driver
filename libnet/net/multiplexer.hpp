/**
 *  @author    Jakob Otto
 *  @file      multiplexer_base.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#if defined(__linux__)
#  include "net/detail/epoll_multiplexer.hpp"
#  define LIB_NET_EPOLL
#elif defined(__APPLE__)
#  include "net/detail/kqueue_multiplexer.hpp"
#  define LIB_NET_KQUEUE
#endif

namespace net {

#if defined(__linux__)
/// @brief Platform-specific multiplexer type alias for Linux (epoll-based).
using multiplexer = ::net::detail::epoll_multiplexer;
#elif defined(__APPLE__)
/// @brief Platform-specific multiplexer type alias for macOS (kqueue-based).
using multiplexer = ::net::detail::kqueue_multiplexer;
#endif

/// @brief Shared pointer type for multiplexer instances.
using multiplexer_ptr = std::shared_ptr<multiplexer>;

/// @brief Creates a new multiplexer instance with the specified configuration.
/// @param factory A manager factory function that creates socket managers.
/// @param cfg The configuration settings for the multiplexer.
/// @return Either a shared pointer to the created multiplexer or an error.
util::error_or<multiplexer_ptr>
make_multiplexer(multiplexer::manager_factory factory, const util::config& cfg);

} // namespace net
