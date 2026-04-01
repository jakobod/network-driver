// /**
//  *  @author    Jakob Otto
//  *  @file      multiplexer.cpp
//  *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
//  *             This file is part of the network-driver project, released
//  under
//  *             the GNU GPL3 License.
//  */

// #if !defined(__linux__)
// #  error "uring multiplexer is only usable on Linux"
// #else

// #  include "net/uring/multiplexer.hpp"

// #  include "net/acceptor.hpp"
// #  include "net/pollset_updater.hpp"

// #  include "net/event_result.hpp"
// #  include "net/ip/v4_address.hpp"
// #  include "net/ip/v4_endpoint.hpp"
// #  include "net/operation.hpp"
// #  include "net/socket/pipe_socket.hpp"
// #  include "net/socket/tcp_accept_socket.hpp"

// #  include "util/binary_serializer.hpp"
// #  include "util/byte_span.hpp"
// #  include "util/config.hpp"
// #  include "util/error.hpp"
// #  include "util/error_or.hpp"
// #  include "util/format.hpp"
// #  include "util/intrusive_ptr.hpp"
// #  include "util/logger.hpp"

// #  include <algorithm>
// #  include <iostream>
// #  include <unistd.h>
// #  include <utility>

// namespace net::uring {

// multiplexer::~multiplexer() {
//   LOG_TRACE();
//   ::close(mpx_fd_);
// }

// util::error multiplexer::init(acceptor::factory_type factory,
//                               const util::config& cfg) {
//   LOG_TRACE();
//   LOG_DEBUG("initializing kqueue multiplexer");
//   mpx_fd_ = ::kqueue();
//   if (mpx_fd_ < 0) {
//     return {util::error_code::runtime_error,
//             "[multiplexer]: Creating epoll fd failed"};
//   }
//   LOG_DEBUG("Created ", NET_ARG(mpx_fd_));
//   return multiplexer_base::init(std::move(factory), cfg);
// }

// void multiplexer::add(manager_base_ptr mgr, operation initial) {
//   LOG_TRACE();
//   if (is_multiplexer_thread()) {
//     LOG_DEBUG("Adding socket_manager with ", NET_ARG2("id",
//     mgr->handle().id),
//               " for ", NET_ARG(initial));
//     // Set nonblocking on socket
//     if (!nonblocking(mgr->handle(), true)) {
//       handle_error(util::error(util::error_code::socket_operation_failed,
//                                "Could not set nonblocking on handle"));
//     }
//     // Add the mgr to the pollset for both reading and writing and enable it
//     for
//     // the initial operations
//     mod(mgr->handle().id, (EV_ADD | EV_DISABLE), operation::read_write);
//     enable(*mgr, initial);
//     managers_.emplace(mgr->handle().id, mgr);
//     // TODO: This should probably return an error instead of calling
//     // handle_error
//     if (auto err = mgr->init(*cfg_)) {
//       handle_error(err);
//     }
//   } else {
//     LOG_DEBUG("Requesting to add socket_manager with ",
//               NET_ARG2("id", mgr->handle().id), " for ", NET_ARG(initial));
//     mgr->ref();
//     write_to_pipe(pollset_updater::add_code, mgr.get(), initial);
//   }
// }

// void multiplexer::enable(manager_base& mgr, operation op) {
//   LOG_TRACE();
//   LOG_DEBUG("Enabling mgr with ", NET_ARG2("id", mgr->handle().id),
//             " registered for ", NET_ARG2("mask", to_string(mgr->mask())),
//             " for ", NET_ARG2("new_event", to_string(op)));
//   if (!mgr.mask_add(op)) {
//     return;
//   }
//   mod(mgr.handle().id, EV_ENABLE, mgr.mask());
// }

// void multiplexer::disable(manager_base& mgr, operation op, bool remove) {
//   LOG_TRACE();
//   LOG_DEBUG("Disabling mgr with ", NET_ARG2("id", mgr->handle().id),
//             " registered for ", NET_ARG2("mask", to_string(mgr->mask())),
//             " for ", NET_ARG2("event", to_string(op)));
//   if (!mgr.mask_del(op)) {
//     return;
//   }
//   mod(mgr.handle().id, EV_DISABLE, op);
//   if (remove && mgr.mask() == operation::none) {
//     del(mgr.handle());
//   }
// }

// void multiplexer::del(socket handle) {
//   LOG_TRACE();
//   LOG_DEBUG("Deleting mgr with ", NET_ARG2("id", handle.id));
//   mod(handle.id, EV_DELETE, operation::read_write);
//   managers_.erase(handle.id);
//   if (shutting_down_ && managers_.empty()) {
//     running_ = false;
//   }
// }

// multiplexer::manager_map::iterator multiplexer::del(manager_map::iterator it)
// {
//   LOG_TRACE();
//   auto fd = it->second->handle().id;
//   LOG_DEBUG("Deleting mgr with ", NET_ARG2("id", fd));
//   mod(fd, EV_DELETE, operation::read_write);
//   auto new_it = managers_.erase(it);
//   if (shutting_down_ && managers_.empty()) {
//     running_ = false;
//   }
//   return new_it;
// }

// void multiplexer::mod(int fd, int op, operation events) {
//   LOG_TRACE();
//   LOG_DEBUG("Modifying mgr with ", NET_ARG2("id", fd), ", ", NET_ARG(op),
//             ", for ", NET_ARG2("events", to_string(events)));
//   static constexpr const auto to_kevent_filter
//     = [](net::operation op) -> int16_t {
//     switch (op) {
//       case net::operation::none:
//         return 0;
//       case net::operation::read:
//         return EVFILT_READ;
//       case net::operation::write:
//         return EVFILT_WRITE;
//       default:
//         return 0;
//     }
//   };

//   // kqueue may not add sockets for both reading and writing in a single
//   call.. if (events == operation::read_write) {
//     mod(fd, op, operation::read);
//     mod(fd, op, operation::write);
//   } else {
//     event_type event;
//     EV_SET(&event, fd, to_kevent_filter(events), op, 0, 0, nullptr);
//     update_cache_.push_back(event);
//   }
// }

// util::error multiplexer::poll_once(bool blocking) {
//   using namespace std::chrono;
//   LOG_TRACE();
//   const auto now = time_point_cast<milliseconds>(steady_clock::now());
//   const auto timeout_passed = (current_timeout_ ? (now > *current_timeout_)
//                                                 : false);
//   // Calculate the timeout value for the kqueue call
//   const auto timeout = std::invoke(
//     [this, blocking, timeout_passed, now] -> timespec {
//       if (!blocking || !current_timeout_ || timeout_passed) {
//         return {0, 0};
//       }
//       const auto timeout_tp =
//       time_point_cast<milliseconds>(*current_timeout_); const auto diff_ms =
//       (timeout_tp - now).count(); return {(diff_ms / 1000), ((diff_ms % 1000)
//       * 1000000)};
//     });

//   LOG_DEBUG("kevent ", NET_ARG(blocking), ", timeout={", timeout.tv_sec,
//   "s,",
//             timeout.tv_nsec, "ns}");

//   // Pass nullptr only when blocking indefinitely (no timeout set)
//   // Otherwise pass &timeout: either {0, 0} for non-blocking/passed timeout
//   or
//   // actual timeout
//   const auto* timeout_ptr = (blocking && !current_timeout_) ? nullptr
//                                                             : &timeout;
//   const int num_events = kevent(mpx_fd_, update_cache_.data(),
//                                 static_cast<int>(update_cache_.size()),
//                                 pollset_.data(),
//                                 static_cast<int>(pollset_.size()),
//                                 timeout_ptr);

//   update_cache_.clear();
//   // Check for errors
//   if (num_events < 0) {
//     return (errno == EINTR) ? util::none
//                             : util::error{util::error_code::runtime_error,
//                                           util::last_error_as_string()};
//   }
//   // Handle all timeouts and io-events that have been registered
//   handle_timeouts();
//   handle_events(event_span(pollset_.data(),
//   static_cast<size_t>(num_events))); return util::none;
// }

// void multiplexer::handle_events(event_span events) {
//   LOG_TRACE();
//   LOG_DEBUG("Handling ", events.size(), " I/O events");
//   auto handle_result = [this](manager_base& mgr, event_result res,
//                               operation op) {
//     LOG_DEBUG(NET_ARG2("res", to_string(res)));
//     switch (res) {
//       case event_result::done:
//         disable(mgr, op, true);
//         break;
//       case event_result::error:
//         del(mgr.handle());
//         break;
//       default:
//         // nop
//         break;
//     }
//   };

//   for (const auto& event : events) {
//     const auto handle = socket{static_cast<net::socket_id>(event.ident)};
//     if (event.flags & EV_EOF) {
//       LOG_ERROR("EV_EOF on ", NET_ARG2("handle", handle.id), ": ",
//                 util::last_error_as_string());
//       del(handle);
//       continue;
//     } else {
//       auto& mgr = *managers_[handle.id];
//       switch (event.filter) {
//         case EVFILT_READ:
//           LOG_DEBUG("Handling EVFILT_READ on manager with ",
//                     NET_ARG2("id", handle.id));
//           handle_result(mgr, mgr.handle_read_event(), operation::read);
//           break;
//         case EVFILT_WRITE:
//           LOG_DEBUG("Handling EVFILT_WRITE on manager with ",
//                     NET_ARG2("id", handle.id));
//           handle_result(mgr, mgr.handle_write_event(), operation::write);
//           break;
//         default:
//           LOG_WARNING("Event filter unknown");
//           break;
//       }
//     }
//   }
// }

// util::error_or<multiplexer_ptr> make_multiplexer(acceptor::factory_type
// factory,
//                                                  const util::config& cfg) {
//   LOG_TRACE();
//   auto mpx = std::make_shared<multiplexer>();
//   if (auto err = mpx->init(std::move(factory), cfg)) {
//     return err;
//   }
//   return mpx;
// }

// } // namespace net::uring

// #endif
