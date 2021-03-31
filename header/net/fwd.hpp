/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 29.03.2021
 */

#pragma once

#include <memory>

namespace net {

class socket_manager;
class multiplexer;

struct socket;
struct stream_socket;
struct tcp_stream_socket;
struct tcp_accept_socket;

using socket_manager_ptr = std::shared_ptr<socket_manager>;

} // namespace net
