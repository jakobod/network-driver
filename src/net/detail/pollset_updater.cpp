/**
 *  @author    Jakob Otto
 *  @file      pollset_updater.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/detail/pollset_updater.hpp"
#include "net/detail/event_handler.hpp"
#include "net/socket/pipe_socket.hpp"

#include "util/logger.hpp"

#if defined(LIB_NET_URING)
#  include "net/detail/uring_manager.hpp"
#  include "net/detail/uring_multiplexer.hpp"
#endif

#include <sys/socket.h>
#include <sys/uio.h>

namespace net::detail {

// -- pollset_updater_base implementation --------------------------------------

template <class ManagerBase>
pollset_updater_base<ManagerBase>::pollset_updater_base(net::pipe_socket handle,
                                                        multiplexer_base* mpx)
  : ManagerBase{handle, mpx} {
  LOG_TRACE();
  // Add the different parts of the message to the iovecs for reading at once
  iov_[0] = iovec{.iov_base = &code_, .iov_len = sizeof(code_)};
  iov_[1] = iovec{.iov_base = &mgr_, .iov_len = sizeof(mgr_)};
  iov_[2] = iovec{.iov_base = &op_, .iov_len = sizeof(op_)};
  // Create a msghdr struct for use in recvmsg
  msg_ = {}; // Zero-initialize
  msg_.msg_iov = iov_.data();
  msg_.msg_iovlen = iov_.size();
}

template <class ManagerBase>
manager_result pollset_updater_base<ManagerBase>::handle_operation() {
  switch (code_) {
    case pollset_opcode::add:
      LOG_DEBUG("Received opcode::add for mgr with ",
                NET_ARG2("id", mgr->handle().id), " with ", NET_ARG(op_));
      ManagerBase::mpx()->add(util::make_intrusive(mgr_, false), op_);
      return manager_result::ok;

    case pollset_opcode::shutdown:
      LOG_DEBUG("Received opcode::shutdown");
      ManagerBase::mpx()->shutdown();
      return manager_result::done;

    default:
      LOG_WARNING("Received unhandled code");
      return manager_result::error;
  }
}

// -- pollset_updater<event_handler> implementation -(epoll/kqueue) -----------

manager_result pollset_updater<event_handler>::handle_read_event() {
  LOG_TRACE();
  // TODO Recvmsg?
  if (const auto res = net::readv(handle<pipe_socket>(), base::iov_);
      res != pollset_updater_base::message_size) {
    LOG_ERROR("Could not read ", pollset_message_size,
              " bytes from pipe socket: ", net::last_socket_error_as_string());
    return manager_result::error;
  }
  return base::handle_operation();
}

#if defined(LIB_NET_URING)

// -- pollset_updater<uring_manager> implementation (io_uring) -----------------

pollset_updater<uring_manager>::pollset_updater(net::pipe_socket handle,
                                                multiplexer_base* mpx)
  : pollset_updater_base{handle, mpx} {
  // nop
}

manager_result pollset_updater<uring_manager>::enable(operation op) {
  if (op != operation::read) {
    LOG_ERROR(
      "pollset_updater enabled for operation other than operation::read");
    return manager_result::error;
  }
  mask_add(operation::read);
  auto [success, submission_id]
    = manager_base::mpx<uring_multiplexer>()->submit_readv(*this, iov_);
  return success ? manager_result::ok : manager_result::error;
}

manager_result pollset_updater<uring_manager>::handle_completion(operation op,
                                                                 int res) {
  if (op != operation::read) {
    LOG_ERROR("pollset_updater called for operation != read");
    return manager_result::error;
  }

  if (res != message_size) {
    LOG_ERROR("Not enough bytes for pollset updater to use");
    return manager_result::error;
  }

  const auto handle_res = base::handle_operation();
  if (handle_res == manager_result::ok) {
    auto [success, submission_id]
      = manager_base::mpx<uring_multiplexer>()->submit_readv(*this, iov_);
    if (!success) {
      return manager_result::error;
    }
  }
  return handle_res;
}

#endif // LIB_NET_URING

// -- Explicit template instantiations -----------------------------------------

/// @brief Instantiate pollset_updater_base for event_handler (epoll/kqueue)
template class pollset_updater_base<event_handler>;

#if defined(LIB_NET_URING)
/// @brief Instantiate pollset_updater_base for uring_manager (io_uring)
template class pollset_updater_base<uring_manager>;
#endif

} // namespace net::detail
