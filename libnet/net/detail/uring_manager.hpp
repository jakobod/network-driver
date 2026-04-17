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

#  include <utility>

struct iovec;

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

  // -- Move and assignment operations -----------------------------------------

  /// @brief Copy construction is deleted.
  uring_manager(const uring_manager&) = delete;

  /// @brief Move constructor.
  uring_manager(uring_manager&& mgr) noexcept = default;

  /// @brief Copy assignment is deleted.
  uring_manager& operator=(const uring_manager&) = delete;

  /// @brief Move assignment.
  uring_manager& operator=(uring_manager&& other) noexcept = default;

  // -- Event handling ---------------------------------------------------------

  virtual manager_result enable(operation op) = 0;

  /// @brief Handles a completed io_uring operation.
  /// Subclasses override this to process completion results.
  /// @param op The operation that completed (read/write/accept).
  /// @param res The IO result (number of bytes or error code).
  /// @param id The id of the submission.
  /// @return manager_result indicating handler status (ok/done/error).
  virtual manager_result
  handle_completion(operation op, int res, std::uint64_t id)
    = 0;
};

/// @brief Shared pointer type for uring managers.
using uring_manager_ptr = util::intrusive_ptr<uring_manager>;

} // namespace net::detail

#endif
