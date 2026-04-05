/**
 *  @author    Jakob Otto
 *  @file      detail/uring_manager.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released
 under
 *             the GNU GPL3 License.
 */

#pragma once

#if defined(LIB_NET_URING)

#  include "net/fwd.hpp"
#  include "util/fwd.hpp"

#  include "net/detail/manager_base.hpp"
#  include "net/event_result.hpp"
#  include "net/receive_policy.hpp"

#  include "util/byte_buffer.hpp"
#  include "util/byte_span.hpp"
#  include "util/error.hpp"
#  include "util/logger.hpp"
#  include "util/ref_counted.hpp"

#  include <liburing.h>
#  include <vector>

namespace net::detail {

/// @brief Socket manager specialized for io_uring operations.
/// Unlike traditional event-driven managers that handle read/write events,
/// uring managers coordinate with the io_uring submission queue and handle
/// completion processing. Manages separate read and write buffers with
/// configurable receive policies.
class uring_manager : public manager_base {
public:
  /// @brief Constructs a uring manager for the given socket.
  /// @param handle The socket to manage.
  /// @param mpx The multiplexer that owns this manager (may be
  /// uring_multiplexer or base).
  uring_manager(socket handle, multiplexer_base* mpx)
    : manager_base{handle, mpx} {
    // nop
  }

  /// @brief Virtual destructor.
  virtual ~uring_manager() = default;

  // -- Move and assignment operations ----------------------------------------

  /// @brief Copy construction is deleted.
  uring_manager(const uring_manager&) = delete;

  /// @brief Move constructor.
  uring_manager(uring_manager&& mgr) noexcept = default;

  /// @brief Copy assignment is deleted.
  uring_manager& operator=(const uring_manager&) = delete;

  /// @brief Move assignment.
  uring_manager& operator=(uring_manager&& other) noexcept = default;

  // -- initialization -------------------------------------------------------

  /// @brief Initializes the manager with the given configuration.
  /// Sets up initial read buffer policy.
  /// @param cfg Configuration parameters.
  /// @return An error on failure, none on success.
  util::error init(const util::config&) override {
    configure_next_read(receive_policy::up_to(1024));
    return util::none;
  }

  // -- Event handling -------------------------------------------------------

  /// @brief Configures the read buffer for the next read operation.
  /// Resizes the buffer if the requested size differs from the current size.
  /// @param policy The receive policy specifying min/max read size.
  void configure_next_read(receive_policy policy) {
    LOG_DEBUG("Configuring next read on ", NET_ARG2("socket", handle().id),
              ": ", NET_ARG(min_read_size_), ", ",
              NET_ARG2("max_read_size_", policy.max_size));
    received_ = 0;
    min_read_size_ = policy.min_size;
    if (read_buffer_.size() != policy.max_size) {
      read_buffer_.resize(policy.max_size);
    }
  }

  /// @brief Handles a completed io_uring operation.
  /// Subclasses override this to process completion results.
  /// @param op The operation that completed (read/write/accept).
  /// @param res The IO result (number of bytes or error code).
  /// @return event_result indicating handler status (ok/done/error).
  virtual event_result handle_completion([[maybe_unused]] operation op,
                                         [[maybe_unused]] int res) {
    return event_result::ok;
  }

  // -- Buffer management ---------------------------------------------------

  /// @brief Returns mutable reference to the input (read) buffer.
  /// @return Reference to the read buffer.
  util::byte_buffer& read_buffer() { return read_buffer_; }

  /// @brief Returns const reference to the input (read) buffer.
  /// @return Const reference to the read buffer.
  const util::byte_buffer& read_buffer() const noexcept { return read_buffer_; }

  /// @brief Returns mutable reference to the output (write) buffer.
  /// @return Reference to the write buffer.
  util::byte_buffer& write_buffer() { return write_buffer_; }

  /// @brief Returns const reference to the output (write) buffer.
  /// @return Const reference to the write buffer.
  const util::byte_buffer& write_buffer() const noexcept {
    return write_buffer_;
  }

  // -- uring_mpx access ---------------------------------------------------

  /// @brief Returns the data ready to be written to the socket.
  /// Used by the uring multiplexer for submission queue entry preparation.
  /// @return A const span of data to write.
  virtual util::const_byte_span data_to_write() const noexcept {
    return write_buffer();
  }

  /// @brief Returns the buffer space available for reading.
  /// Accounts for data already received into the buffer.
  /// @return A span of the available buffer space.
  virtual util::byte_span data_to_read() noexcept {
    return std::span{read_buffer_.begin() + received_, read_buffer_.end()};
  }

protected:
  std::size_t min_read_size_{0}; ///< Minimum bytes to consider read complete
  std::size_t received_{0};      ///< Current position in read buffer

private:
  util::byte_buffer read_buffer_;  ///< Input (read) buffer
  util::byte_buffer write_buffer_; ///< Output (write) buffer
};

/// @brief Shared pointer type for uring managers.
using uring_manager_ptr = util::intrusive_ptr<uring_manager>;

} // namespace net::detail

#endif
