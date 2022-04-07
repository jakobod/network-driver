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

// -- net namespace forward declarations ---------------------------------------

namespace net {

class acceptor;
class epoll_multiplexer;
class multiplexer;
class pollset_updater;
class socket_manager;
class socket_manager_factory;

struct datagram_socket;
struct pipe_socket;
struct raw_socket;
struct socket;
struct stream_socket;
struct tcp_stream_socket;
struct tcp_accept_socket;
struct timeout_entry;

enum class event_result : uint8_t;
enum class operation : uint32_t;

using datagram_socket_pair = std::pair<datagram_socket, datagram_socket>;
using stream_socket_pair = std::pair<stream_socket, stream_socket>;

using multiplexer_ptr = std::shared_ptr<multiplexer>;
using socket_manager_ptr = std::shared_ptr<socket_manager>;
using socket_manager_factory_ptr = std::shared_ptr<socket_manager_factory>;

template <class Socket>
class socket_guard;

} // namespace net

// -- net::ip namespace forward declarations -----------------------------------

namespace net::ip {

class v4_address;

}

// -- util namespace forward declarations --------------------------------------

namespace util {

struct error;

class binary_deserializer;
class binary_serializer;
class serialized_size;

template <class T>
using error_or = std::variant<T, error>;
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
constexpr byte_span make_byte_span(T& container) noexcept {
  return {reinterpret_cast<std::byte*>(container.data()), container.size()};
}

template <class T>
constexpr const_byte_span make_const_byte_span(const T& container) noexcept {
  return {reinterpret_cast<const std::byte*>(container.data()),
          container.size()};
}

} // namespace util
