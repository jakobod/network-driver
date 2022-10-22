/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/acceptor.hpp"

#include "net/event_result.hpp"
#include "net/multiplexer.hpp"
#include "net/socket_manager_factory.hpp"
#include "net/tcp_accept_socket.hpp"

#include "util/error.hpp"
#include "util/error_code.hpp"
#include "util/logger.hpp"

#include <iostream>
#include <memory>

namespace net {

acceptor::acceptor(tcp_accept_socket handle, multiplexer* mpx,
                   socket_manager_factory_ptr factory)
  : socket_manager(handle, mpx), factory_(std::move(factory)) {
  LOG_TRACE();
}

util::error acceptor::init() {
  LOG_TRACE();
  return util::none;
}

// -- properties -------------------------------------------------------------

event_result acceptor::handle_read_event() {
  auto hdl = handle<tcp_accept_socket>();
  LOG_TRACE();
  LOG_DEBUG("acceptor handling read event ", NET_ARG2("handle", hdl.id));
  auto accepted = accept(hdl);
  if (accepted == invalid_socket) {
    mpx()->handle_error({util::error_code::socket_operation_failed,
                         "[acceptor::handle_read_event()] accept failed: {0}",
                         last_socket_error_as_string()});
    return event_result::ok;
  }
  LOG_DEBUG("accepted connection ", NET_ARG2("new_handle", accepted.id));
  if (!nonblocking(accepted, true)) {
    mpx()->handle_error(
      {util::error_code::socket_operation_failed,
       "[acceptor::handle_read_event()] nonblocking failed: {0}",
       last_socket_error_as_string()});
    return event_result::ok;
  }
  auto mgr = factory_->make(accepted, mpx());
  mpx()->add(std::move(mgr), operation::read);
  return event_result::ok;
}

event_result acceptor::handle_write_event() {
  LOG_ERROR("Should not be registered for write_events");
  mpx()->handle_error({util::error_code::runtime_error,
                       "[acceptor::handle_write_event()] acceptor should not "
                       "be registered for writing"});
  return event_result::error;
}

event_result acceptor::handle_timeout(uint64_t) {
  LOG_ERROR("Should not be registered for timeouts");
  mpx()->handle_error({util::error_code::runtime_error,
                       "[acceptor::handle_timeout()] not implemented!"});
  return event_result::error;
}

} // namespace net
