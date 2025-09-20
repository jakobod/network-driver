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

constexpr std::uint32_t default_uring_queue_depth = 512;

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
  LOG_DEBUG("initializing uring_multiplexer");
  LOG_TRACE();
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
  accept_socket_                   = accept_socket;
  port_                            = port;
  LOG_DEBUG("listening on ", NET_ARG2("port", port_));

  enqueue_accept();
  return util::none;
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
  LOG_DEBUG("multiplexer shutting down");
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

util::error uring_multiplexer::add(sockets::tcp_stream_socket handle) {
  LOG_TRACE();
  // LOG_DEBUG("Adding socket_manager with ", NET_ARG2("id", mgr->handle().id),
  //           " for ", NET_ARG(initial));
  auto mgr = manager_factory_->make_uring_manager(handle, this);
  if (const auto err = mgr->init(*cfg_)) {
    return err;
  }
  managers_.emplace(handle.id, std::move(mgr));
  return util::none;
}

util::error uring_multiplexer::add(sockets::udp_datagram_socket handle) {
  LOG_TRACE();
  // LOG_DEBUG("Adding socket_manager with ", NET_ARG2("id", mgr->handle().id),
  //           " for ", NET_ARG(initial));
  std::ignore = handle;

  // auto* read_info   = new uring_entry{initial, mgr.get()};
  // io_uring_sqe* sqe = io_uring_get_sqe(&uring_handle_);
  // auto& read_buffer = mgr->read_buffer();
  // io_uring_prep_recv(sqe, mgr->handle, read_info->buffer.data(),
  //                    read_info->buffer.size(), 0);
  // io_uring_sqe_set_data(sqe, read_info);
  // io_uring_submit(&uring_handle_);
  // mgr.release();
  return util::none;
}

std::uint64_t
uring_multiplexer::set_timeout(manager_ptr,
                               std::chrono::steady_clock::time_point) {
  DEBUG_ONLY_ASSERT(false, "not implemented yet");
  return 0;
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

void uring_multiplexer::enqueue_accept() {
  auto* sqe  = io_uring_get_sqe(&uring_handle_);
  auto* data = new uring_entry{operation::accept, nullptr};
  io_uring_prep_accept(sqe, accept_socket_.id, nullptr, nullptr, 0);
  io_uring_sqe_set_data(sqe, data);
  io_uring_submit(&uring_handle_);
}

void uring_multiplexer::enqueue_read(uring_manager_ptr mgr,
                                     util::byte_span receive_buffer) {
  auto* sqe = io_uring_get_sqe(&uring_handle_);
  io_uring_prep_recv(sqe, mgr->handle().id, receive_buffer.data(),
                     receive_buffer.size(), 0);
  auto* read_info = new uring_entry{operation::read, mgr};
  io_uring_sqe_set_data(sqe, read_info);
  io_uring_submit(&uring_handle_);
}

void uring_multiplexer::enqueue_write(uring_manager_ptr mgr,
                                      util::const_byte_span write_buffer) {
  auto* sqe = io_uring_get_sqe(&uring_handle_);
  io_uring_prep_write(sqe, mgr->handle().id, write_buffer.data(),
                      write_buffer.size(), 0);
  auto* read_info = new uring_entry{operation::write, mgr};
  io_uring_sqe_set_data(sqe, read_info);
  io_uring_submit(&uring_handle_);
}

util::error uring_multiplexer::poll_once(bool) {
  using namespace std::chrono;
  LOG_TRACE();

  io_uring_cqe* cqe = nullptr;
  LOG_DEBUG("Waiting for handled events");
  const auto ret = io_uring_wait_cqe(&uring_handle_, &cqe);
  if (ret < 0) {
    LOG_ERROR("io_uring_wait_cqe failed");
    return util::error{util::error_code::socket_operation_failed,
                       "io_uring_wait_cque failed"};
  }

  if (auto* entry
      = reinterpret_cast<uring_entry*>(io_uring_cqe_get_data(cqe))) {
    auto [op, mgr] = *entry;
    delete entry; // TODO: this can be made prettier?

    auto handle_event_result = [this](operation op, event_result res,
                                      const manager_ptr& mgr) {
      std::ignore = op;
      LOG_DEBUG("Handling event_result from mgr handler ", NET_ARG(op),
                NET_ARG(res), NET_ARG2("handle", mgr->handle().id));
      if ((res == event_result::error) || (res == event_result::done)) {
        del(mgr->handle());
      }
    };

    switch (op) {
      case operation::accept: {
        sockets::tcp_stream_socket accepted_socket{cqe->res};
        std::ignore = add(accepted_socket); // TODO
        enqueue_accept();
        break;
      }

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
