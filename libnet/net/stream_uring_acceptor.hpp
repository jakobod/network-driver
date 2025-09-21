/**
 *  @author    Jakob Otto
 *  @file      stream_uring_acceptor.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "net/event_result.hpp"
#include "net/receive_policy.hpp"
#include "net/uring_manager.hpp"

#include "net/sockets/tcp_accept_socket.hpp"
#include "net/sockets/tcp_stream_socket.hpp"

#include "util/assert.hpp"
#include "util/config.hpp"
#include "util/error.hpp"
#include "util/logger.hpp"

#include <cstring>

struct io_uring_cqe;

namespace net {

class stream_uring_acceptor : public uring_manager {
public:
  stream_uring_acceptor(sockets::tcp_accept_socket handle,
                        uring_multiplexer* mpx)
    : uring_manager{handle, mpx} {
    LOG_TRACE();
  }

  /// Destructs a socket manager object
  ~stream_uring_acceptor() override { LOG_TRACE(); }

  util::error init(const util::config&) override {
    LOG_TRACE();
    enqueue_accept_sqe();
    return util::none;
  }

  void configure_next_read(receive_policy) override {
    LOG_TRACE();
    DEBUG_ONLY_ASSERT(false, "Function not implemented");
  }

  void start_reading() override {
    LOG_TRACE();
    DEBUG_ONLY_ASSERT(false, "Function not implemented");
  }

  void start_writing() override {
    LOG_TRACE();
    DEBUG_ONLY_ASSERT(false, "Function not implemented");
  }

  /// Handles a timeout-event
  event_result handle_timeout(std::uint64_t) override {
    LOG_TRACE();
    DEBUG_ONLY_ASSERT(false, "Function not implemented");
    return event_result::error;
  }

  event_result handle_read_completion(const io_uring_cqe* cqe) override {
    LOG_TRACE();
    DEBUG_ONLY_ASSERT(cqe != nullptr, "");
    if (cqe->res <= 0) {
      LOG_ERROR("Error during accept: ", std::strerror(-cqe->res));
      return event_result::error;
    }
    sockets::tcp_stream_socket accepted_socket{cqe->res};
    LOG_DEBUG("Accepted ", NET_ARG2("handle", accepted_socket.id));
    if (const auto err = mpx<uring_multiplexer>()->add(accepted_socket)) {
      LOG_ERROR(err);
      return event_result::error;
    }
    enqueue_accept_sqe();
    return event_result::ok;
  }

  event_result handle_write_completion(const io_uring_cqe*) override {
    LOG_TRACE();
    DEBUG_ONLY_ASSERT(false, "Function not implemented");
    return event_result::error;
  }

private:
  void enqueue_accept_sqe() { mpx<uring_multiplexer>()->enqueue_accept(this); }
};

using stream_uring_acceptor_ptr = util::intrusive_ptr<stream_uring_acceptor>;

} // namespace net
