/**
 *  @author    Jakob Otto
 *  @file      stream_uring_manager.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "net/application.hpp"
#include "net/event_result.hpp"
#include "net/uring_manager.hpp"

#include "net/sockets/tcp_stream_socket.hpp"

#include "util/assert.hpp"
#include "util/error.hpp"
#include "util/logger.hpp"

#include "meta/concepts.hpp"

struct io_uring_cqe;

namespace net {

template <meta::derived_from<application> ApplicationType>
class stream_uring_manager : public uring_manager {
public:
  template <class... Args>
  stream_uring_manager(sockets::tcp_stream_socket handle,
                       uring_multiplexer* mpx, Args&&... args)
    : uring_manager{handle, mpx},
      application_{this, std::forward<Args>(args)...} {
    LOG_TRACE();
  }

  /// Destructs a socket manager object
  ~stream_uring_manager() override { LOG_TRACE(); }

  util::error init(const util::config& cfg) override {
    LOG_TRACE();
    if (auto err = application_.init(cfg)) {
      return err;
    }
    start_reading();
    return util::none;
  }

  void configure_next_read(receive_policy policy) override {
    LOG_TRACE();
    received_ = 0;
    read_buffer_.resize(policy.max_size);
    min_read_size_ = policy.min_size;
  }

  void start_reading() override {
    LOG_TRACE();
    if (mask_add(operation::read)) {
      LOG_DEBUG("Started Reading");
      enqueue_read_sqe();
    }
  }

  void start_writing() override {
    LOG_TRACE();
    if (mask_add(operation::write)) {
      LOG_DEBUG("Started Writing");
      enqueue_write_sqe();
    }
  }

  /// Handles a timeout-event
  event_result handle_timeout(std::uint64_t timeout_id) override {
    LOG_TRACE();
    application_.handle_timeout(timeout_id);
    return event_result::ok;
  }

  event_result handle_read_completion(const io_uring_cqe* cqe) override {
    DEBUG_ONLY_ASSERT(cqe != nullptr, "");
    LOG_TRACE();
    if (cqe->res < 0) {
      LOG_ERROR("error while receiving data");
      return event_result::error;
    } else if (cqe->res == 0) {
      LOG_DEBUG("peer disconnected");
      return event_result::done;
    } else {
      received_ += cqe->res;
      if (received_ >= min_read_size_) {
        if (auto err = application_.consume(
              util::byte_span{read_buffer_.data(), received_})) {
          LOG_ERROR(err);
          return event_result::error;
        }
      }
      enqueue_read_sqe(); // Rearm the reading
      return event_result::ok;
    }
  }

  event_result handle_write_completion(const io_uring_cqe* cqe) override {
    LOG_TRACE();
    if (cqe->res <= 0) {
      LOG_ERROR("[stream_uring_manager]: Failed to write data");
      return event_result::error;
    }

    // Update the current write_view and check whether new data must be enqueued
    write_view_ = write_view_.subspan(cqe->res);
    enqueue_write_sqe();
    return event_result::ok;
  }

private:
  void enqueue_read_sqe() {
    LOG_TRACE();
    mpx<uring_multiplexer>()->enqueue_recv(
      this, {read_buffer_.data() + received_, read_buffer_.size() - received_});
  }

  void swap_output_buffers() noexcept {
    LOG_TRACE();
    network_buffer_.clear();
    std::swap(network_buffer_, write_buffer_);
    write_view_ = util::byte_span{network_buffer_};
  }

  bool must_swap_output_buffers() const noexcept {
    LOG_TRACE();
    return write_view_.empty() && !write_buffer_.empty();
  }

  void enqueue_write_sqe() {
    LOG_TRACE();
    if (must_swap_output_buffers()) {
      swap_output_buffers();
    }

    // Enqueue a new sqe only if we actually have any data to write
    if (!write_view_.empty()) {
      mpx<uring_multiplexer>()->enqueue_write(this, write_view_);
    } else {
      mask_del(operation::write);
    }
  }

  util::byte_buffer network_buffer_;
  util::byte_span write_view_;
  ApplicationType application_;
};

} // namespace net
