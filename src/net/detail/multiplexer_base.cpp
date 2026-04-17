/**
 *  @author    Jakob Otto
 *  @file      multiplexer_base.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/detail/multiplexer_base.hpp"

#include "net/detail/event_handler.hpp"
#include "net/detail/pollset_updater.hpp"

#include "net/socket/tcp_accept_socket.hpp"

#include "net/ip/v4_address.hpp"
#include "net/ip/v4_endpoint.hpp"

#include "util/config.hpp"
#include "util/error_or.hpp"
#include "util/logger.hpp"

namespace net::detail {

// -- Thread functions -------------------------------------------------------

/// Creates a thread that runs this multiplexer indefinately.
void multiplexer_base::start() {
  LOG_TRACE();
  if (!running_) {
    running_ = true;
    mpx_thread_ = std::thread(&multiplexer_base::run, this);
    mpx_thread_id_ = mpx_thread_.get_id();
    LOG_DEBUG(NET_ARG(mpx_thread_id_));
  }
}

void multiplexer_base::run() {
  LOG_TRACE();
  while (running_) {
    auto err = poll_once(true);
    if (err) {
      running_ = false;
    }
  }
}

/// Shuts the multiplexer down!
void multiplexer_base::shutdown() {
  LOG_TRACE();
  if (is_multiplexer_thread()) {
    LOG_DEBUG("multiplexer shutting down");
    shutting_down_ = true;
    auto it = managers_.begin();
    while (it != managers_.end()) {
      auto& mgr = it->second;
      if (mgr->handle() != pipe_reader_) {
        disable(*mgr, (operation::read_accept), false);
        if (mgr->mask() == operation::none) {
          it = del(it);
        } else {
          ++it;
        }
      } else {
        ++it;
      }
    }
    close(pipe_writer_);
    pipe_writer_ = pipe_socket{};
  } else if (!shutting_down_) {
    LOG_DEBUG("requesting multiplexer shutdown");
    auto res = write_to_pipe(pollset_opcode::shutdown, nullptr,
                             operation::none);
    if (res != 10) {
      LOG_ERROR("could not write shutdown code to pipe: ",
                last_socket_error_as_string());
      std::terminate(); // Can't be handled by shutting down, if shutdown fails.
    }
  }
}

/// Joins with the multiplexer.
void multiplexer_base::join() {
  LOG_TRACE();
  if (mpx_thread_.joinable()) {
    LOG_DEBUG("joining on multiplexer thread");
    mpx_thread_.join();
  }
}

bool multiplexer_base::is_running() const noexcept {
  return mpx_thread_.joinable();
}

void multiplexer_base::set_thread_id(std::thread::id tid) noexcept {
  mpx_thread_id_ = tid;
}

// -- Timeout management -------------------------------------------------------

std::uint64_t
multiplexer_base::set_timeout(manager_base& mgr,
                              std::chrono::steady_clock::time_point when) {
  LOG_TRACE();
  LOG_DEBUG("Setting timeout ", current_timeout_id_, " on ",
            NET_ARG2("mgr", mgr.handle().id));
  timeouts_.emplace(mgr.handle().id, when, current_timeout_id_);
  current_timeout_ = current_timeout_ ? std::min(when, *current_timeout_)
                                      : when;
  return current_timeout_id_++;
}

void multiplexer_base::handle_timeouts() {
  LOG_TRACE();
  // Swap with cached set to reuse allocated memory
  cached_timeouts_.swap(timeouts_);

  // Process all expired timeouts and collect unhandled ones
  auto it = cached_timeouts_.begin();
  while (it != cached_timeouts_.end()) {
    if (it->has_expired()) {
      // Registered timeout has expired
      auto& mgr = managers_.at(it->handle());
      mgr->handle_timeout(it->id());
      ++it;
    } else {
      break;
    }
  }

  // Move unhandled timeouts back with any newly added timeouts
  timeouts_.insert(it, cached_timeouts_.end());
  cached_timeouts_.clear();

  // Update current timeout to the next expiring timeout, if any
  if (timeouts_.empty()) {
    LOG_DEBUG("No further timeouts registered");
    current_timeout_ = std::nullopt;
  } else {
    LOG_DEBUG("Next timeout with ", NET_ARG2("id", timeouts_.begin()->id()));
    current_timeout_ = timeouts_.begin()->when();
  }
}

// -- Error handling -----------------------------------------------------------

void multiplexer_base::handle_error([[maybe_unused]] util::error err) {
  LOG_ERROR(err);
  shutdown();
}

} // namespace net::detail
