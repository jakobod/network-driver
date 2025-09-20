/**
 *  @author    Jakob Otto
 *  @file      uring_manager_impl.hpp
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

#include "util/assert.hpp"
#include "util/error.hpp"
#include "util/logger.hpp"

#include "meta/concepts.hpp"

struct io_uring_cqe;

namespace net {

template <meta::derived_from<application> ApplicationType>
class uring_manager_impl : public uring_manager {
public:
  template <class... Args>
  uring_manager_impl(sockets::socket handle, uring_multiplexer* mpx,
                     Args&&... args)
    : uring_manager{handle, mpx},
      application_{this, std::forward<Args>(args)...} {
    LOG_TRACE();
  }

  /// Destructs a socket manager object
  ~uring_manager_impl() override = default;

  util::error init(const util::config& cfg) override {
    return application_.init(cfg);
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
    std::ignore = timeout_id;
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
      return event_result::error;
    }

    written_ += cqe->res;
    enqueue_write_sqe();
    return event_result::ok;
  }

private:
  void enqueue_read_sqe() {
    mpx<uring_multiplexer>()->enqueue_read(
      this, {read_buffer_.data() + received_, read_buffer_.size() - received_});
  }

  void enqueue_write_sqe() {
    if ((network_buffer_.empty()) && !write_buffer_.empty()) {
      // We got more data to write, swap buffers and update the view!
      network_buffer_.clear();
      written_ = 0;
      std::swap(network_buffer_, write_buffer_);
    }

    // Enqueue a new sqe only if we actually have any data to write
    if (network_buffer_.size() == written_) {
      network_buffer_.clear();
      mask_del(operation::write);
    } else {
      mpx<uring_multiplexer>()->enqueue_write(
        this,
        {network_buffer_.data() + written_, network_buffer_.size() - written_});
    }
  }

  ApplicationType application_;
  util::byte_buffer network_buffer_;
};

} // namespace net
