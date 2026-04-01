// /**
//  *  @author    Jakob Otto
//  *  @file      multiplexer_impl.hpp
//  *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
//  *             This file is part of the network-driver project, released
//  under
//  *             the GNU GPL3 License.
//  */

// #pragma once

// #if !defined(__linux__)
// #  error "uring multiplexer is only usable on Linux"
// #else

// #  include "net/fwd.hpp"
// #  include "util/fwd.hpp"

// #  include "net/multiplexer_base.hpp"

// #  include "net/acceptor.hpp"

// #  include <array>
// #  include <cstdint>
// #  include <span>
// #  include <vector>

// #  include <liburing.h>

// namespace net::uring {

// /// Implements a multiplexing backend for handling event multiplexing
// facilities
// /// such as epoll and kqueue.
// class multiplexer : public multiplexer_base {
//   static constexpr std::size_t max_uring_depth = 32;

// public:
//   // -- constructors, destructors
//   ----------------------------------------------

//   multiplexer() = default;

//   virtual ~multiplexer();

//   /// Initializes the multiplexer.
//   util::error init(acceptor::factory_type factory, const util::config& cfg);

//   // -- Interface functions
//   ----------------------------------------------------

//   void add(manager_base_ptr mgr, operation initial) override;

//   /// Enables an operation `op` for socket manager `mgr`.
//   void enable(manager_base& mgr, operation op) override;

//   /// Disables an operation `op` for socket manager `mgr`.
//   /// If `mgr` is not registered for any operation after disabling it, it is
//   /// removed if `remove` is set.
//   void disable(manager_base& mgr, operation op, bool remove) override;

//   /// Main multiplexing loop.
//   util::error poll_once(bool blocking) override;

// private:
//   /// Handles all IO-events that occurred.
//   void handle_events(event_span events);

//   /// Deletes an existing socket_manager using its key `handle`.
//   void del(socket handle);

//   /// Deletes an existing socket_manager using an iterator `it` to the
//   /// manager_map.
//   manager_map::iterator del(manager_map::iterator it) override;

//   /// Modifies the epollset for existing fds.
//   void mod(int fd, int op, operation events);

//   // Multiplexing variables
//   io_uring uring;
// };

// using multiplexer_ptr = std::shared_ptr<multiplexer>;

// util::error_or<multiplexer_ptr> make_multiplexer(acceptor::factory_type
// factory,
//                                                  const util::config& cfg);

// } // namespace net::uring

// #endif
