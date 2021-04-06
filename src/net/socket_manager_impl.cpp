/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 30.03.2021
 *
 *  This file is based on `socket_manager.cpp` from the C++ Actor Framework.
 *  https://github.com/actor-framework/incubator
 */

#include "net/socket_manager_impl.hpp"

#include <iostream>
#include <memory>

#include "detail/byte_container.hpp"
#include "net/tcp_stream_socket.hpp"

namespace net {

socket_manager_impl::socket_manager_impl(socket handle, multiplexer* mpx)
  : socket_manager(handle, mpx) {
  // nop
}

socket_manager_impl::~socket_manager_impl() {
  // nop
}

bool socket_manager_impl::handle_read_event() {
  // TODO: Dummy implementation!
  detail::byte_array<2048> buf;
  auto ret = read(socket_cast<tcp_stream_socket>(handle()), buf);
  std::string_view str{reinterpret_cast<char*>(buf.data()),
                       static_cast<size_t>(ret)};
  std::cerr << str << std::endl;
  return true; // FALSE JUST IN CASE OF ERROR!
}

bool socket_manager_impl::handle_write_event() {
  // TODO: write from the socket
  return false;
}

} // namespace net
