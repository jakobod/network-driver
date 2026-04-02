/**
 *  @author    Jakob Otto
 *  @file      detail/uring_manager.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released
 under
 *             the GNU GPL3 License.
 */

#if !defined(__linux__)
#  error "uring_manager is only usable on Linux"
#elif !defined(LIB_NET_URING)
#  error "uring support not enabled"
#else

#  include "net/detail/uring_manager.hpp"

#  include "net/detail/uring_multiplexer.hpp"

#  include "net/event_result.hpp"

#  include "util/config.hpp"
#  include "util/error.hpp"
#  include "util/logger.hpp"

namespace net::detail {

event_result uring_manager::handle_cqe(const io_uring_cqe* cqe) {
  LOG_TRACE();
  if (!cqe) {
    LOG_WARNING("Received null completion queue entry");
    return event_result::error;
  }

  LOG_DEBUG("Handling uring data: res=", NET_ARG(cqe->res),
            " flags=", NET_ARG(cqe->flags));

  // Default implementation: process the data result
  // This can be overridden in subclasses for specific behavior
  if (cqe->res < 0) {
    LOG_ERROR("io_uring operation failed with error code: ", -cqe->res);
    return event_result::error;
  }

  // Successful operation - return done by default
  // Subclasses can override to provide custom logic
  return event_result::done;
}

} // namespace net::detail

#endif
