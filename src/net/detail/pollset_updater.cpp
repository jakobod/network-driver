/**
 *  @author    Jakob Otto
 *  @file      pollset_updater.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/detail/pollset_updater.hpp"
#include "net/detail/event_handler.hpp"
#include "net/detail/multiplexer_base.hpp"
#include "net/socket/pipe_socket.hpp"

#include "util/binary_deserializer.hpp"
#include "util/logger.hpp"

#if defined(LIB_NET_URING)
#  include "net/detail/uring_manager.hpp"
#  include "net/detail/uring_multiplexer.hpp"
#endif

namespace net::detail {

// -- pollset_updater_base implementation --------------------------------------

template <class ManagerBase>
pollset_updater_base<ManagerBase>::pollset_updater_base(net::pipe_socket handle,
                                                        multiplexer_base* mpx)
  : ManagerBase{handle, mpx} {
  LOG_TRACE();
}

template <class ManagerBase>
event_result
pollset_updater_base<ManagerBase>::handle_operation(pollset_opcode code,
                                                    ManagerBase* mgr,
                                                    operation op) {
  switch (code) {
    case pollset_opcode::add:
      LOG_DEBUG("Received opcode::add for mgr with ",
                NET_ARG2("id", mgr->handle().id), " with ", NET_ARG(op));
      ManagerBase::mpx()->add(util::make_intrusive(mgr, false), op);
      return event_result::ok;

    case pollset_opcode::shutdown:
      LOG_DEBUG("Received opcode::shutdown");
      ManagerBase::mpx()->shutdown();
      return event_result::done;

    default:
      LOG_WARNING("Received unhandled code");
      return event_result::error;
  }
}

// -- pollset_updater<event_handler> implementation -(epoll/kqueue) -----------

event_result pollset_updater<event_handler>::handle_read_event() {
  LOG_TRACE();
  auto code = pollset_opcode::none;
  event_handler* mgr = nullptr;
  operation op;

  if (read_from_pipe(code)) {
    return event_result::error;
  }
  if (read_from_pipe(mgr)) {
    return event_result::error;
  }
  if (read_from_pipe(op)) {
    return event_result::error;
  }
  return base::handle_operation(code, mgr, op);
}

#if defined(LIB_NET_URING)

// -- pollset_updater<uring_manager> implementation (io_uring) -----------------

pollset_updater<uring_manager>::pollset_updater(net::pipe_socket handle,
                                                multiplexer_base* mpx)
  : pollset_updater_base{handle, mpx} {
  read_buffer_.resize(10);
}

event_result pollset_updater<uring_manager>::handle_completion(operation op,
                                                               int res) {
  if (op != operation::read) {
    LOG_ERROR("pollset_updater called for operation != read");
    return event_result::error;
  }

  if (res != 10) {
    LOG_ERROR("Not enough bytes for pollset updater to use");
    return event_result::ok;
  }

  auto code = pollset_opcode::none;
  uring_manager* mgr = nullptr;
  operation op_result;
  util::binary_deserializer deserializer(read_buffer_);
  deserializer(code, mgr, op_result);
  const auto handle_res = base::handle_operation(code, mgr, op_result);
  if (handle_res == event_result::ok) {
    manager_base::mpx<uring_multiplexer>()->submit(this, operation::read);
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
