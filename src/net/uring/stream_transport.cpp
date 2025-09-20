/**
 *  @author    Jakob Otto
 *  @file      stream_transport.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/uring/stream_transport.hpp"

#include "net/event_result.hpp"
#include "net/sockets/socket.hpp"

#include "util/assert.hpp"
#include "util/config.hpp"
#include "util/error.hpp"

#include <iostream>

namespace {} // namespace

namespace net::uring {

void stream_transport::configure_next_read(receive_policy policy)

  void stream_transport::enqueue_read_sqe()

    void stream_transport::enqueue_write_sqe()

      void stream_transport::swap_write_buffers() noexcept {
  std::swap(write_buffer_, network_buffer_);
}

} // namespace net::uring
