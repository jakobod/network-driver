/**
 *  @author    Jakob Otto
 *  @file      uring_manager.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "net/uring/manager.hpp"

#include "net/event_result.hpp"
#include "net/operation.hpp"
#include "net/receive_policy.hpp"
#include "net/sockets/socket.hpp"
#include "net/uring/multiplexer_impl.hpp"

#include "util/byte_buffer.hpp"
#include "util/error.hpp"
#include "util/logger.hpp"

struct io_uring_cqe;

namespace net::uring {

/// Manages the lifetime of a socket and its events.
template <class Application>
class stream_transport : public manager {
public:
  template <class... Ts>
  stream_transport(sockets::socket handle, multiplexer_impl* mpx, Ts&&... xs)
    : manager{handle, mpx}, application_{std::forward<Ts>(xs)...} {
    // nop
  }

  /// Destructs a socket manager object
  ~stream_transport() override = default;

  /// Initializes a uring_manager
  util::error init(const util::config& cfg) override {
    if (auto res = application_.init(*this, cfg)) {
      return res;
    }
    enqueue_read_sqe();
    return util::none;
  }

  // -- Application Layer Interface --------------------------------------------

  util::byte_buffer& get_write_buffer() noexcept { return write_buffer_; }

  void configure_next_read(receive_policy policy) {
    received_      = 0;
    min_read_size_ = policy.min_size;
    receive_buffer_.resize(policy.max_size);
  }

  // -- event handling ---------------------------------------------------------

  void start_reading() {
    LOG_TRACE();
    if (mask_add(operation::read)) {
      LOG_DEBUG("Started Reading");
      enqueue_read_sqe();
    }
  }

  void start_writing() {
    LOG_TRACE();
    if (mask_add(operation::write)) {
      LOG_DEBUG("Started Writing");
      enqueue_write_sqe();
    }
  }

  /// Handles a read-event
  event_result handle_read_completion(const io_uring_cqe& cqe) override {
    LOG_TRACE();
    if (cqe.res < 0) {
      LOG_ERROR("error while receiving data");
      return event_result::error;
    } else if (cqe.res == 0) {
      LOG_DEBUG("peer disconnected");
      return event_result::done;
    } else {
      received_ += cqe.res;
      if (received_ >= min_read_size_) {
        if (auto err = application_.consume(
              *this, util::byte_span{receive_buffer_.data(), received_})) {
          LOG_ERROR(err);
          return event_result::error;
        }
      }
      enqueue_read_sqe(); // Rearm the reading
      return event_result::ok;
    }
  }

  /// Handles a write-event
  event_result handle_write_completion(const io_uring_cqe& cqe) override {
    LOG_TRACE();
    if (cqe.res <= 0) {
      LOG_ERROR("error while sending data: ", -cqe.res, ", \"",
                std::strerror(-cqe.res), "\"");
      return event_result::error;
    }

    written_ += cqe.res;
    enqueue_write_sqe();
    return event_result::ok;
  }

private:
  void enqueue_read_sqe() {
    mpx()->enqueue_read(this, {receive_buffer_.data() + received_,
                               receive_buffer_.size() - received_});
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
      mpx()->enqueue_write(this, {network_buffer_.data() + written_,
                                  network_buffer_.size() - written_});
    }
  }

  std::size_t received_{0};
  std::size_t min_read_size_{0};
  util::byte_buffer receive_buffer_;

  /// @brief Buffer that applications use to append data to the transport
  util::byte_buffer write_buffer_;
  /// @brief The buffer that is currently being written to the network
  util::byte_buffer network_buffer_;
  /// @brief
  std::size_t written_{0};

  Application application_;
};

} // namespace net::uring
