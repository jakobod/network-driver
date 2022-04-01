/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <memory>
#include <span>
#include <variant>
#include <vector>

namespace net {

class socket_manager;
class socket_manager_factory;
class acceptor;
class pollset_updater;
class multiplexer;
class epoll_multiplexer;
class multithreaded_epoll_multiplexer;

struct socket;
struct raw_socket;
struct stream_socket;
struct pipe_socket;
struct tcp_stream_socket;
struct tcp_accept_socket;
struct timeout_entry;
struct error;

enum class operation : uint32_t;
enum class event_result : uint8_t;

using stream_socket_pair = std::pair<stream_socket, stream_socket>;
using socket_manager_ptr = std::shared_ptr<socket_manager>;
using socket_manager_factory_ptr = std::shared_ptr<socket_manager_factory>;
using multiplexer_ptr = std::shared_ptr<multiplexer>;

template <class T>
using error_or = std::variant<T, error>;
template <class Socket>
class socket_guard;

} // namespace net

namespace util {

class binary_deserializer;
class binary_serializer;
class serialized_size;

template <class Func>
class scope_guard;

using byte_buffer = std::vector<std::byte>;
using byte_span = std::span<std::byte>;
using const_byte_span = std::span<const std::byte>;

template <size_t Size>
using byte_array = std::array<std::byte, Size>;

// -- helper functions ---------------------------------------------------------

template <typename... Ts>
constexpr std::array<std::byte, sizeof...(Ts)>
make_byte_array(Ts&&... args) noexcept {
  return {std::byte(std::forward<Ts>(args))...};
}

template <class T>
constexpr std::span<T> make_span(T* ptr, std::size_t size) noexcept {
  return {ptr, size};
}

} // namespace util
