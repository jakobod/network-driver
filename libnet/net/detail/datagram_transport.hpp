/**
 *  @author    Jakob Otto
 *  @file      datagram_transport.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"

#include "net/detail/event_handler.hpp"
#include "net/detail/multiplexer_base.hpp"
#include "net/detail/transport_base.hpp"
#if defined(LIB_NET_URING)
#  include "net/detail/uring_manager.hpp"
#  include "net/detail/uring_multiplexer.hpp"
#endif

#include "net/manager_result.hpp"
#include "net/receive_policy.hpp"

#include "net/socket/datagram_socket.hpp"
#include "net/socket/udp_datagram_socket.hpp"

#include "net/ip/v4_endpoint.hpp"

#include "util/assert.hpp"
#include "util/config.hpp"
#include "util/error.hpp"
#include "util/format.hpp"
#include "util/logger.hpp"

#include <algorithm>
#include <ranges>
#include <sys/uio.h>
#include <utility>

namespace net::detail {

struct datagram {
  datagram(util::byte_buffer buf, ip::v4_endpoint ep)
    : buf_{std::move(buf)}, ep_{std::move(ep)} {
    // nop
  }

  util::byte_buffer buf_;
  ip::v4_endpoint ep_;
  std::uint64_t id_{0};
  bool currently_enqueued_{false};
};

// -- datagram_transport implementation ----------------------------------------

/// @brief Layered datagram (UDP) transport implementation.
/// Provides a transport layer for datagram-based protocols (UDP) with
/// buffer management for reading and writing. Supports layering other
/// protocol handlers on top through the NextLayer template parameter.
/// @tparam NextLayer The upper protocol layer to stack on this transport.
template <class ManagerBase, class NextLayer>
class datagram_transport : public transport_base, public ManagerBase {
protected:
  // TODO Possibly add querying the MTU from the socket?
  static constexpr std::size_t max_datagram_size = 548;

public:
  /// @brief Constructs a stream transport layer.
  /// @param handle The stream socket for this connection.
  /// @param mpx The multiplexer managing this socket.
  /// @param args Constructor arguments forwarded to NextLayer.
  template <class... Args>
  datagram_transport(udp_datagram_socket handle, detail::multiplexer_base* mpx,
                     Args&&... args)
    : ManagerBase{handle, mpx}, next_layer_{std::forward<Args>(args)...} {
    LOG_DEBUG("Creating datagram_transport with ", NET_ARG2("id", handle.id));
  }

  /// @brief Initializes the transport with configuration.
  /// Sets the socket to non-blocking mode and initializes the next layer.
  /// @param cfg The configuration object.
  /// @return An error if initialization fails, success otherwise.
  util::error init(const util::config& cfg) override {
    LOG_DEBUG("init datagram_transport with ", NET_ARG2("socket", handle().id));
    if (auto err = ManagerBase::init(cfg)) {
      return err;
    }
    if (auto err = transport_base::init(cfg)) {
      return err;
    }
    return next_layer_.init(*this, cfg);
  }

  // -- manager_base API -------------------------------------------------------

  manager_result handle_timeout(uint64_t id) override {
    return next_layer_.handle_timeout(*this, id);
  }

  // -- transport_base API -----------------------------------------------------

  void configure_next_read(receive_policy policy) noexcept override {
    LOG_DEBUG("Configuring next read on ", NET_ARG2("socket", handle().id),
              ": ", NET_ARG(min_read_size_), ", ",
              NET_ARG2("max_read_size_", policy.max_size));
    received_ = 0;
    min_read_size_ = policy.min_size;
    read_buffer_.resize(policy.max_size);
  }

  // -- datagram_transport specific API ----------------------------------------

  void enqueue(util::byte_buffer&& datagram, ip::v4_endpoint ep) {
    // Warn about datagrams that are too big?
    write_queue_.emplace_back(std::move(datagram), std::move(ep));
    manager_base::register_writing();
  }

  void enqueue(util::const_byte_span datagram, ip::v4_endpoint ep) {
    auto buf = transport_base::get_buffer();
    buf.assign(datagram.begin(), datagram.end());
    enqueue(std::move(buf), std::move(ep));
  }

protected:
  manager_result handle_read_result(const udp_read_result res) {
    const auto [ep, read_res] = res;
    if (read_res < 0) {
      // Check whether the error is temporary, i.e., EAGAIN
      return last_socket_error_is_temporary() ? manager_result::temporary_error
                                              : manager_result::error;
    } else if (read_res == 0) {
      return manager_result::done; // Disconnect
    } else {
      LOG_DEBUG("Read ", read_res, " bytes from ",
                NET_ARG2("socket", handle().id));
      received_ += read_res;
      if (received_ >= min_read_size_) {
        const auto consume_result = next_layer_.consume(
          *this, util::const_byte_span{read_buffer_.data(), received_}, ep);
        if (consume_result == manager_result::error) {
          return manager_result::error;
        }
        received_ = 0;
      }
      return manager_result::ok;
    }
  }

  manager_result handle_write_result(int write_res, std::uint64_t id) {
    // Really necessary to search for packets with the id? They should be
    // handled in order of submission, also with uring.
    auto it = std::ranges::find_if(write_queue_, [id](const auto& datagram) {
      return datagram.id_ == id;
    });
    if (it == write_queue_.end()) {
      LOG_ERROR("Datagram with ", NET_ARG(id), " was not found in write_queue");
      return manager_result::error;
    }
    auto& datagram = *it;

    if (write_res < 0) {
      if (last_socket_error_is_temporary()) {
        datagram.currently_enqueued_ = false;
        return manager_result::temporary_error;
      } else {
        return manager_result::error;
      }
    }
    LOG_DEBUG("Wrote ", write_res, " bytes to ",
              NET_ARG2("socket", handle().id));

    if (static_cast<std::size_t>(write_res) < datagram.buf_.size()) {
      LOG_ERROR("Datagram with ", NET_ARG(id), " was not written completely");
      return manager_result::error;
    }
    datagram.buf_.clear();
    transport_base::return_buffer(std::move(datagram.buf_));
    write_queue_.erase(it);
    return manager_result::ok;
  }

  bool done_writing() const noexcept {
    return (write_queue_.empty() && !next_layer_.has_more_data());
  }

  manager_result fetch_more_data() {
    size_t i = 0;
    while ((num_enqueued_bytes_ < transport_base::max_enqueued_bytes_)
           && (i < transport_base::max_consecutive_fetches_)) {
      if (next_layer_.has_more_data()) {
        next_layer_.produce(*this);
      } else {
        break;
      }
      ++i;
    }
    return done_writing() ? manager_result::done : manager_result::ok;
  }

  std::span<iovec> iovecs() const noexcept { return iovecs_; }

  /// @brief Returns the buffer space available for reading.
  /// @return A span of the available buffer space.
  util::byte_span read_buffer() noexcept {
    return std::span{read_buffer_.data() + received_,
                     read_buffer_.size() - received_};
  }

protected:
  NextLayer next_layer_;

  size_t received_{0};
  size_t written_{0};
  size_t min_read_size_{0};

  size_t num_enqueued_bytes_{0};

  util::byte_buffer read_buffer_;
  mutable std::deque<datagram> write_queue_;
  mutable std::vector<iovec> iovecs_;
};

template <class ManagerBase, class NextLayer>
class datagram_transport_impl;

/// @brief Specialization for event_handler (epoll/kqueue).
template <class NextLayer>
class datagram_transport_impl<event_handler, NextLayer>
  : public datagram_transport<event_handler, NextLayer> {
  using base = datagram_transport<event_handler, NextLayer>;

public:
  using base::base;

  /// @brief Handles incoming connection on read event (epoll/kqueue).
  manager_result handle_read_event() override {
    LOG_TRACE();
    LOG_DEBUG("handle read_event on ", NET_ARG2("socket", handle().id));
    for (size_t i = 0; i < base::max_consecutive_reads_; ++i) {
      auto read_buffer = base::read_buffer();
      if (read_buffer.size() == 0) {
        LOG_WARNING("Datagram read_buffer has size 0");
        return manager_result::ok;
      }
      const auto read_res = read(base::template handle<udp_datagram_socket>(),
                                 base::read_buffer());
      const auto verdict = base::handle_read_result(read_res);
      if (verdict != manager_result::ok) {
        return verdict;
      }
    }
    return manager_result::ok;
  }

  /// @brief Handles incoming connection on read event (epoll/kqueue).
  manager_result handle_write_event() override {
    LOG_TRACE();
    LOG_DEBUG("handle write_event on ", NET_ARG2("socket", handle().id));

    size_t num_consecutive_writes = 0;
    do {
      // Trigger the next layers to generate some data to write
      if (base::fetch_more_data() == manager_result::done) {
        return manager_result::done;
      }
      for (auto& datagram : base::write_queue_) {
        datagram.id_ = 42069;
        const auto write_res
          = write(manager_base::handle<udp_datagram_socket>(), datagram.buf_,
                  datagram.ep_);
        const auto verdict = base::handle_write_result(write_res, 42069);
        if ((verdict == manager_result::error)
            || (verdict == manager_result::temporary_error)) {
          return verdict;
        }
      }
    } while (num_consecutive_writes++ < base::max_consecutive_writes_);
    return base::done_writing() ? manager_result::done : manager_result::ok;
  }
};

template <class NextLayer>
using event_datagram_transport
  = datagram_transport_impl<event_handler, NextLayer>;

#if defined(LIB_NET_URING)

/// @brief Specialization for uring_manager (io_uring).
template <class NextLayer>
class datagram_transport<uring_manager, NextLayer>
  : public datagram_transport_base<uring_manager, NextLayer> {
  using base = datagram_transport_base<uring_manager, NextLayer>;

public:
  using base::base;

  manager_result handle_completion(operation op, int res) override {
    LOG_TRACE();
    LOG_DEBUG("Handling ", NET_ARG(op), " with ", NET_ARG(res), " on ",
              NET_ARG2("handle", handle().id));
    switch (op) {
      case operation::read: {
        const auto verdict = base::handle_read_result(res);
        if (verdict == manager_result::temporary_error) {
          uring_manager::submit(operation::poll_read);
        }
        if (verdict != manager_result::ok) {
          return verdict;
        }
      }
        [[fallthrough]];
      case operation::poll_read:
        uring_manager::submit(operation::read);
        return manager_result::ok;

      case operation::write: {
        const auto verdict = base::handle_write_result(res);
        if (verdict == manager_result::temporary_error) {
          uring_manager::submit(operation::poll_write);
          return manager_result::done;
        } else if (verdict != manager_result::ok) {
          return verdict;
        }
      }
        [[fallthrough]];
      case operation::poll_write:
        base::fetch_more_data();
        if (base::done_writing()) {
          return manager_result::done;
        }
        uring_manager::submit(operation::write);
        return manager_result::ok;

      default:
        LOG_ERROR(NET_ARG(op), " not handled by uring_datagram_transport");
        return manager_result::error;
    }
  }

  /// @brief Returns the buffer space available for reading.
  /// @return A span of the available buffer space.
  util::byte_span read_buffer() noexcept override {
    return std::span{base::read_buffer_.data() + base::received_,
                     base::read_buffer_.size() - base::received_};
  }

  /// @brief Returns the buffer with the data currently queued for writing.
  /// @return A span of the data queued for writing.
  std::span<iovec> write_buffer() const noexcept override {
    return base::iovecs();
  }
};

template <class NextLayer>
using uring_datagram_transport = datagram_transport<uring_manager, NextLayer>;

#endif

} // namespace net::detail
