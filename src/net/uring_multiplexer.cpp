/**
 *  @author    Jakob Otto
 *  @file      uring_multiplexer.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/uring_multiplexer.hpp"

#include "net/event_result.hpp"
#include "net/manager.hpp"
#include "net/manager_factory.hpp"
#include "net/operation.hpp"
#include "net/stream_uring_acceptor.hpp"

#include "net/ip/v4_address.hpp"
#include "net/ip/v4_endpoint.hpp"

#include "net/sockets/tcp_accept_socket.hpp"
#include "net/sockets/tcp_stream_socket.hpp"
#include "net/sockets/udp_datagram_socket.hpp"

#include "util/assert.hpp"
#include "util/byte_span.hpp"
#include "util/config.hpp"
#include "util/error.hpp"
#include "util/error_or.hpp"
#include "util/intrusive_ptr.hpp"
#include "util/logger.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <thread>
#include <utility>

namespace {

constexpr std::uint32_t default_uring_queue_depth = 2048;

} // namespace

namespace net {

uring_multiplexer::uring_multiplexer(manager_factory_ptr manager_factory)
  : manager_factory_{std::move(manager_factory)} {
  LOG_TRACE();
}

uring_multiplexer::~uring_multiplexer() {
  LOG_TRACE();
  shutdown();
  join();
}

util::error uring_multiplexer::init(const util::config& cfg) {
  LOG_TRACE();
  LOG_DEBUG("initializing uring_multiplexer");
  cfg_ = std::addressof(cfg);

  const auto queue_depth = cfg_->get_or<std::int64_t>(
    "multiplexer.uring_queue_depth", default_uring_queue_depth);
  if (auto res = io_uring_queue_init(static_cast<std::uint32_t>(queue_depth),
                                     &uring_handle_, 0);
      res < 0) {
    // LOG_ERROR("Failed to initialize uring", NET_ARG(res),
    //           NET_ARG2("error", std::strerror(-res)));
    return util::error{util::error_code::runtime_error,
                       "Failed to initialize uring"};
  }

  // Create Acceptor
  const auto res = sockets::make_tcp_accept_socket(ip::v4_endpoint(
    (cfg_->get_or("multiplexer.local", true) ? ip::v4_address::localhost
                                             : ip::v4_address::any),
    cfg_->get_or<std::int64_t>("multiplexer.port", 0)));
  if (auto err = util::get_error(res)) {
    return *err;
  }
  const auto [accept_socket, port] = std::get<sockets::acceptor_pair>(res);
  port_                            = port;
  LOG_DEBUG("listening on ", NET_ARG2("port", port_));
  return add(accept_socket);
}

void uring_multiplexer::start() {
  LOG_TRACE();
  if (!running_) {
    running_    = true;
    mpx_thread_ = std::jthread(&uring_multiplexer::run, this);
  }
}

void uring_multiplexer::shutdown() {
  LOG_TRACE();
  // TODO

  // auto it = managers_.begin();
  // while (it != managers_.end()) {
  //   auto& mgr = it->second;
  //   disable(mgr, operation::read, false);
  //   if (mgr->mask() == operation::none) {
  //     it = del(it);
  //   } else {
  //     ++it;
  //   }
  // }
  // shutting_down_ = true;
  // close(pipe_writer_);
  // pipe_writer_ = sockets::pipe_socket{};
}

void uring_multiplexer::join() {
  LOG_TRACE();
  if (mpx_thread_.joinable()) {
    LOG_DEBUG("joining on multiplexer thread");
    mpx_thread_.join();
  }
}

void uring_multiplexer::run() {
  LOG_TRACE();
  while (running_) {
    if (poll_once(true)) {
      // stop in case of error.
      running_ = false;
    }
  }
}

// -- Error handling -----------------------------------------------------------

util::error uring_multiplexer::add(sockets::tcp_accept_socket handle) {
  LOG_TRACE();
  LOG_DEBUG("Adding tcp_accept_socket with ", NET_ARG2("id", handle.id));
  auto mgr = manager_factory_->make_uring_manager(handle, this);
  ASSERT(mgr != nullptr, "Factory returned nullptr");
  return add(std::move(mgr));
}

util::error uring_multiplexer::add(sockets::tcp_stream_socket handle) {
  LOG_TRACE();
  LOG_DEBUG("Adding tcp_stream_socket with ", NET_ARG2("id", handle.id));
  auto mgr = manager_factory_->make_uring_manager(handle, this);
  ASSERT(mgr != nullptr, "Factory returned nullptr");
  return add(std::move(mgr));
}

util::error uring_multiplexer::add(sockets::udp_datagram_socket handle) {
  LOG_TRACE();
  LOG_DEBUG("Adding udp_datagram_socket with ", NET_ARG2("id", handle.id));
  auto mgr = manager_factory_->make_uring_manager(handle, this);
  ASSERT(mgr != nullptr, "Factory returned nullptr");
  return add(std::move(mgr));
}

util::error uring_multiplexer::add(uring_manager_ptr mgr) {
  auto* mgr_ptr = mgr.get();
  managers_.emplace(mgr->handle().id, std::move(mgr));
  if (const auto err = mgr_ptr->init(*cfg_)) {
    return err;
  }
  return util::none;
}

std::uint64_t
uring_multiplexer::set_timeout(manager_ptr mgr,
                               std::chrono::steady_clock::time_point when) {
  LOG_TRACE();
  LOG_DEBUG("Setting timeout ", current_timeout_id_, " on ",
            NET_ARG2("mgr", mgr->handle().id));
  timeouts_.emplace(mgr->handle().id, when, current_timeout_id_);
  current_timeout_ = (current_timeout_ != std::nullopt)
                       ? std::min(when, *current_timeout_)
                       : when;
  return current_timeout_id_++;
}

void uring_multiplexer::del(sockets::socket handle) {
  LOG_TRACE();
  LOG_DEBUG("Deleting mgr with ", NET_ARG2("id", handle.id));

  // TODO: remove manager alltogetger

  managers_.erase(handle.id);
  if (shutting_down_ && managers_.empty()) {
    running_ = false;
  }
}

uring_multiplexer::manager_map::iterator
uring_multiplexer::del(manager_map::iterator it) {
  LOG_TRACE();
  // auto fd = it->second->handle().id;
  // LOG_DEBUG("Deleting mgr with ", NET_ARG2("id", fd));

  // TODO: remove manager alltogetger

  auto new_it = managers_.erase(it);
  if (shutting_down_ && managers_.empty()) {
    running_ = false;
  }
  return new_it;
}

void uring_multiplexer::enqueue_accept(uring_manager_ptr mgr) {
  LOG_TRACE();
  LOG_DEBUG("Enqueuing accept_sqe for mgr with ",
            NET_ARG2("id", mgr->handle().id));
  auto* sqe = io_uring_get_sqe(&uring_handle_);
  io_uring_prep_accept(sqe, mgr->handle().id, nullptr, nullptr, 0);
  auto* data = new uring_entry{operation::accept, std::move(mgr)};
  io_uring_sqe_set_data(sqe, data);
  if (const auto res = io_uring_submit(&uring_handle_); res < 0) {
    LOG_ERROR("Failed to submit sqe to uring ",
              NET_ARG2("error", std::strerror(-res)));
  }
}

void uring_multiplexer::enqueue_recv(uring_manager_ptr mgr,
                                     util::byte_span receive_buffer) {
  LOG_TRACE();
  LOG_DEBUG("Enqueueing mgr for recv ", NET_ARG2("handle", mgr->handle().id),
            " ", NET_ARG2("num_bytes_to_read", receive_buffer.size()));
  auto* sqe = io_uring_get_sqe(&uring_handle_);
  io_uring_prep_recv(sqe, mgr->handle().id, receive_buffer.data(),
                     receive_buffer.size(), 0);
  auto* read_info = new uring_entry{operation::read, std::move(mgr)};
  io_uring_sqe_set_data(sqe, read_info);
  if (const auto res = io_uring_submit(&uring_handle_); res < 0) {
    LOG_ERROR("Failed to submit sqe to uring ",
              NET_ARG2("error", std::strerror(-res)));
  }
}

void uring_multiplexer::enqueue_recvmsg(uring_manager_ptr mgr,
                                        util::byte_span receive_buffer) {
  // LOG_TRACE();
  // LOG_DEBUG("Enqueueing mgr for recvmsg ", NET_ARG2("handle",
  // mgr->handle().id),
  //           " ", NET_ARG2("num_bytes_to_read", receive_buffer.size()));
  // auto* sqe = io_uring_get_sqe(&uring_handle_);
  // iovec iov{.iov_base = receive_buffer.data(),
  //           .iov_len  = receive_buffer.size()};
  // struct sockaddr_storage src;
  // struct msghdr msg {};
  // msg.msg_name    = &src;
  // msg.msg_namelen = sizeof(src);
  // msg.msg_iov     = &iov;
  // msg.msg_iovlen  = 1;

  // io_uring_prep_recvmsg(sqe, udp_fd, &msg, 0);
  // io_uring_sqe_set_data(sqe, &msg); // optional, to find buffer later
  // io_uring_submit(&ring);
  // if (const auto res = io_uring_submit(&uring_handle_); res < 0) {
  //   LOG_ERROR("Failed to submit sqe to uring ",
  //             NET_ARG2("error", std::strerror(-res)));
  // }
}

void uring_multiplexer::enqueue_write(uring_manager_ptr mgr,
                                      util::const_byte_span write_buffer) {
  LOG_TRACE();
  LOG_DEBUG("Enqueueing mgr for writing ", NET_ARG2("handle", mgr->handle().id),
            " ", NET_ARG2("num_bytes_to_write", write_buffer.size()));
  auto* sqe = io_uring_get_sqe(&uring_handle_);
  io_uring_prep_write(sqe, mgr->handle().id, write_buffer.data(),
                      write_buffer.size(), 0);
  auto* read_info = new uring_entry{operation::write, std::move(mgr)};
  io_uring_sqe_set_data(sqe, read_info);
  if (const auto res = io_uring_submit(&uring_handle_); res < 0) {
    LOG_ERROR("Failed to submit sqe to uring ",
              NET_ARG2("error", std::strerror(-res)));
  }
}

util::error uring_multiplexer::poll_once(bool blocking) {
  using namespace std::chrono;
  LOG_TRACE();

  // TODO: This is a bug. multiple threads will use the same timespec
  static const auto timeout = [&]() -> __kernel_timespec* {
    static __kernel_timespec ts;
    if (!blocking) {
      // Nonblocking
      ts.tv_sec  = 0;
      ts.tv_nsec = 0;
      return &ts;
    }

    if (!current_timeout_) {
      return nullptr; // Indefinite blocking
    }

    // Timed wait until a deadline
    using namespace std::chrono;
    auto now        = steady_clock::now();
    auto deadline   = *current_timeout_;
    const auto diff = (deadline > now) ? deadline - now
                                       : steady_clock::duration{0};

    ts.tv_sec  = duration_cast<seconds>(diff).count();
    ts.tv_nsec = duration_cast<nanoseconds>(diff % seconds(1)).count();
    return &ts;
  };

  io_uring_cqe* cqe = nullptr;
  LOG_DEBUG("Waiting for handled events");
  const auto ret = io_uring_wait_cqe_timeout(&uring_handle_, &cqe, timeout());
  if (ret < 0) {
    LOG_ERROR("io_uring_wait_cqe failed");
    return util::error{util::error_code::socket_operation_failed,
                       "io_uring_wait_cque failed"};
  }

  if (auto* entry
      = reinterpret_cast<uring_entry*>(io_uring_cqe_get_data(cqe))) {
    const auto& [op, mgr]    = *entry;
    auto handle_event_result = [this](operation op, event_result res,
                                      const uring_manager_ptr& mgr) {
      LOG_DEBUG("Handling event_result from mgr handler ", NET_ARG(op), " ",
                NET_ARG(res), " ", NET_ARG2("handle", mgr->handle().id));
      if ((res == event_result::error) || (res == event_result::done)) {
        del(mgr->handle());
      }
    };

    switch (op) {
      case operation::accept:
        mgr->handle_read_completion(cqe);
        break;

      case operation::read: {
        const auto res = mgr->handle_read_completion(cqe);
        handle_event_result(op, res, mgr);
        break;
      }

      case operation::write: {
        const auto res = mgr->handle_write_completion(cqe);
        handle_event_result(op, res, mgr);
        break;
      }

      default:
        DEBUG_ONLY_ASSERT(false, "Unhandled operation type");
        break;
    }
    delete entry;
    io_uring_cqe_seen(&uring_handle_, cqe);
  }

  return util::none;
}

// util::error_or<multiplexer_ptr>
// make_uring_multiplexer(socket_manager_factory_ptr factory,
//                       const util::config& cfg) {
//   LOG_TRACE();
//   auto mpx = std::make_shared<uring_multiplexer>();
//   if (auto err = mpx->init(std::move(factory), cfg)) {
//     return err;
//   }
//   return mpx;
// }

} // namespace net
