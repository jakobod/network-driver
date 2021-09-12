/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace net {

class socket_manager;
class multiplexer;
class socket_manager_factory;

struct socket;
struct stream_socket;
struct pipe_socket;
struct error;
struct tcp_stream_socket;
struct tcp_accept_socket;

enum class operation : uint32_t;

using stream_socket_pair = std::pair<stream_socket, stream_socket>;
using socket_manager_ptr = std::shared_ptr<socket_manager>;
using socket_manager_factory_ptr = std::shared_ptr<socket_manager_factory>;
using multiplexer_ptr = std::shared_ptr<multiplexer>;

} // namespace net

namespace detail {

using byte_buffer = std::vector<std::byte>;
using byte_span = std::span<std::byte>;
using const_byte_span = std::span<const std::byte>;

template <size_t Size>
using byte_array = std::array<std::byte, Size>;

} // namespace detail
