/**
 *  @author    Jakob Otto
 *  @file      detail/uring_manager.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released
 under
 *             the GNU GPL3 License.
 */

#pragma once

#if !defined(__linux__)
#  error "uring_manager is only usable on Linux"
#elif !defined(LIB_NET_URING)
#  error "uring support not enabled"
#else

#  include "net/fwd.hpp"
#  include "util/fwd.hpp"

#  include "net/detail/manager_base.hpp"
#  include "net/event_result.hpp"

#  include "util/byte_buffer.hpp"
#  include "util/byte_span.hpp"
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
  using manager_base::manager_base;

  /// Destructs a uring_manager object
  virtual ~uring_manager() = default;

  // -- Move and assignment operations -----------------------------------------

  uring_manager(const uring_manager&) = delete;
  uring_manager(uring_manager&& mgr) noexcept = default;
  uring_manager& operator=(const uring_manager&) = delete;
  uring_manager& operator=(uring_manager&& other) noexcept = default;

  // -- Event handling ---------------------------------------------------------

  /// Handle data from completed uring operations
  virtual event_result handle_completion([[maybe_unused]] operation op,
                                         [[maybe_unused]] int res) {
    currently_writing_ = false;
    return event_result::ok;
  }

  // -- Data management --------------------------------------------------------

  /// Returns mutable reference to the read buffer
  util::byte_buffer& read_buffer() { return read_buffer_; }

  const util::byte_buffer& read_buffer() const noexcept { return read_buffer_; }

  /// Returns mutable reference to the write buffer
  util::byte_buffer& write_buffer() { return write_buffer_; }

  /// Returns mutable reference to the write buffer
  const util::byte_buffer& write_buffer() const noexcept {
    return write_buffer_;
  }

  /// Returns mutable reference to the write buffer
  util::byte_buffer& read_buffer_for_submission() {
    currently_reading_ = true;
    return read_buffer();
  }

  /// Returns mutable reference to the write buffer
  util::byte_buffer& write_buffer_for_submission() {
    currently_writing_ = true;
    return write_buffer();
  }

  /// Provides data to the uring manager for processing
  virtual bool has_more_data() const noexcept {
    return !write_buffer().empty() && !currently_writing_;
  }

private:
  util::byte_buffer read_buffer_;
  util::byte_buffer write_buffer_;

  bool currently_reading_{false};
  bool currently_writing_{false};
};

using uring_manager_ptr = util::intrusive_ptr<uring_manager>;

} // namespace net::detail

#endif
