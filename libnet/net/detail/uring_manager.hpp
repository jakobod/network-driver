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

/// Manages io_uring specific operations and data handling.
/// Unlike regular managers, uring_manager doesn't handle standard read/write
/// events, but instead manages uring-specific submission queue entries and
/// provides data handling via handle_data().
class uring_manager : public manager_base {
public:
  uring_manager(socket handle, multiplexer_base* mpx)
    : manager_base{handle, mpx} {
    // nop
  }

  /// Destructs a uring_manager object
  virtual ~uring_manager() = default;

  // -- Move and assignment operations -----------------------------------------

  uring_manager(const uring_manager&) = delete;
  uring_manager(uring_manager&& mgr) noexcept = default;
  uring_manager& operator=(const uring_manager&) = delete;
  uring_manager& operator=(uring_manager&& other) noexcept = default;

  // -- initialization ---------------------------------------------------------

  util::error init(const util::config&) override {
    configure_next_read(receive_policy::up_to(1024));
    return util::none;
  }

  // -- Event handling ---------------------------------------------------------

  /// Configures the amount to be read next
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

  /// Handle data from completed uring operations
  virtual event_result handle_completion([[maybe_unused]] operation op,
                                         [[maybe_unused]] int res) {
    return event_result::ok;
  }

  // -- Buffer management ------------------------------------------------------

  /// Returns mutable reference to the read buffer
  util::byte_buffer& read_buffer() { return read_buffer_; }

  const util::byte_buffer& read_buffer() const noexcept { return read_buffer_; }

  /// Returns mutable reference to the write buffer
  util::byte_buffer& write_buffer() { return write_buffer_; }

  /// Returns mutable reference to the write buffer
  const util::byte_buffer& write_buffer() const noexcept {
    return write_buffer_;
  }

  // -- uring_mpx access -------------------------------------------------------

  // Lock this somehow, possibly a second buffer and swap?
  virtual util::const_byte_span data_to_write() const noexcept {
    return write_buffer();
  }

  virtual util::byte_span data_to_read() noexcept {
    return std::span{read_buffer_.begin() + received_, read_buffer_.end()};
  }

protected:
  std::size_t min_read_size_{0};
  std::size_t received_{0};

private:
  util::byte_buffer read_buffer_;
  util::byte_buffer write_buffer_;
};

using uring_manager_ptr = util::intrusive_ptr<uring_manager>;

} // namespace net::detail

#endif
