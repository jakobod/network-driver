// /**
//  *  @author    Jakob Otto
//  *  @file      datagram_transport.hpp
//  *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
//  *             This file is part of the network-driver project, released
//  under
//  *             the GNU GPL3 License.
//  */

// #pragma once

// #include "net/fwd.hpp"

// #include "net/detail/event_handler.hpp"
// #include "net/detail/multiplexer_base.hpp"
// #include "net/detail/transport_base.hpp"
// #if defined(LIB_NET_URING)
// #  include "net/detail/uring_manager.hpp"
// #endif

// #include "net/event_result.hpp"
// #include "net/receive_policy.hpp"
// #include "net/socket/udp_datagram_socket.hpp"

// #include "util/config.hpp"
// #include "util/error.hpp"
// #include "util/format.hpp"
// #include "util/logger.hpp"

// #include <sys/uio.h>
// #include <utility>

// namespace net::detail {

// // -- datagram_transport_base implementation
// -----------------------------------

// /// @brief Layered stream (TCP) transport implementation.
// /// Provides a transport layer for stream-based protocols (TCP) with
// /// buffer management for reading and writing. Supports layering other
// /// protocol handlers on top through the NextLayer template parameter.
// /// @tparam NextLayer The upper protocol layer to stack on this transport.
// template <class ManagerBase, class NextLayer>
// class datagram_transport_base : public transport_base, public ManagerBase {
// public:
//   /// @brief Constructs a stream transport layer.
//   /// @param handle The stream socket for this connection.
//   /// @param mpx The multiplexer managing this socket.
//   /// @param xs Constructor arguments forwarded to NextLayer.
//   template <class... Ts>
//   datagram_transport_base(udp_datagram_socket handle,
//                           detail::multiplexer_base* mpx, Ts&&... xs)
//     : ManagerBase{handle, mpx}, next_layer_(*this, std::forward<Ts>(xs)...) {
//     LOG_DEBUG("Creating datagram_transport with ", NET_ARG2("id",
//     handle.id));
//   }

//   /// @brief Initializes the transport with configuration.
//   /// Sets the socket to non-blocking mode and initializes the next layer.
//   /// @param cfg The configuration object.
//   /// @return An error if initialization fails, success otherwise.
//   util::error init(const util::config& cfg) override {
//     LOG_DEBUG("init stream_transport with ", NET_ARG2("socket",
//     handle().id)); if (auto err = ManagerBase::init(cfg)) {
//       return err;
//     }
//     if (auto err = transport_base::init(cfg)) {
//       return err;
//     }
//     return next_layer_.init(cfg);
//   }

//   // -- manager_base API
//   -------------------------------------------------------

//   event_result handle_timeout(uint64_t id) override {
//     return next_layer_.handle_timeout(id);
//   }

//   // -- transport_base API
//   -----------------------------------------------------

//   void configure_next_read(receive_policy policy) noexcept override {
//     LOG_DEBUG("Configuring next read on ", NET_ARG2("socket", handle().id),
//               ": ", NET_ARG(min_read_size_), ", ",
//               NET_ARG2("max_read_size_", policy.max_size));
//     min_read_size_ = policy.min_size;
//     read_buffer_.resize(read_buffer_.size() + policy.max_size);
//   }

//   // -- stream_transport specific API
//   ------------------------------------------

//   void enqueue(util::const_byte_span data) override {
//     write_buffer_.insert(write_buffer_.end(), data.begin(), data.end());
//   }

// protected:
//   event_result handle_read_result(int read_res) {
//     if (read_res < 0) {
//       // Check whether the error is temporary, i.e., EAGAIN
//       return last_socket_error_is_temporary() ? event_result::temporary_error
//                                               : event_result::error;
//     } else if (read_res == 0) {
//       return event_result::done; // Disconnect
//     } else {
//       LOG_DEBUG("Read ", read_res, " bytes from ",
//                 NET_ARG2("socket", handle().id));
//       received_ += read_res;
//       if (received_ >= min_read_size_) {
//         const auto consume_result = next_layer_.consume(
//           util::const_byte_span{read_buffer_.data(), received_});
//         if (consume_result == event_result::error) {
//           return event_result::error;
//         }
//         received_ = 0;
//       }
//       return event_result::ok;
//     }
//   }

//   void fetch_more_data() const {
//     for (size_t i = 0; i < max_consecutive_fetches_; ++i) {
//       if (next_layer_.has_more_data()) {
//         next_layer_.produce();
//       } else {
//         break;
//       }
//     }
//   }

//   bool done_writing() const noexcept {
//     return (write_buffer_.empty() && !next_layer_.has_more_data());
//   }

//   mutable NextLayer next_layer_; // The next protocol layer in the stack.

//   size_t received_{0};
//   size_t written_{0};
//   size_t min_read_size_{0};

//   util::byte_buffer read_buffer_;
//   mutable util::byte_buffer write_buffer_;
// };

// template <class ManagerBase, class NextLayer>
// class datagram_transport;

// /// @brief Specialization for event_handler (epoll/kqueue).
// template <class NextLayer>
// class datagram_transport<event_handler, NextLayer>
//   : public datagram_transport_base<event_handler, NextLayer> {
//   using base = datagram_transport_base<event_handler, NextLayer>;

// public:
//   using base::base;

//   /// @brief Handles incoming connection on read event (epoll/kqueue).
//   event_result handle_read_event() override {
//     LOG_TRACE();
//     LOG_DEBUG("handle read_event on ", NET_ARG2("socket", handle().id));
//     for (size_t i = 0; i < base::max_consecutive_reads_; ++i) {
//       auto* data = base::read_buffer_.data() + base::received_;
//       const auto size = base::read_buffer_.size() - base::received_;
//       const auto read_res = read(base::template handle<stream_socket>(),
//                                  std::span(data, size));
//       const auto verdict = base::handle_read_result(read_res);
//       if (verdict != event_result::ok) {
//         return verdict;
//       }
//     }
//     return event_result::ok;
//   }

//   /// @brief Handles incoming connection on read event (epoll/kqueue).
//   event_result handle_write_event() override {
//     LOG_TRACE();
//     LOG_DEBUG("handle write_event on ", NET_ARG2("socket", handle().id));

//     size_t num_consecutive_writes = 0;
//     do {
//       // Trigger the next layers to generate some data to write
//       base::fetch_more_data();
//       if (base::done_writing()) {
//         return event_result::done;
//       }

//       const auto write_res = write(base::template handle<stream_socket>(),
//                                    base::write_buffer_);
//       if (write_res < 0) {
//         if (last_socket_error_is_temporary()) {
//           return event_result::ok;
//         } else {
//           return event_result::error;
//         }
//       }

//       LOG_DEBUG("Wrote ", write_res, " bytes to ",
//                 NET_ARG2("socket", handle().id));
//       base::write_buffer_.erase(base::write_buffer_.begin(),
//                                 base::write_buffer_.begin() + write_res);
//     } while (num_consecutive_writes++ < base::max_consecutive_writes_);
//     return base::done_writing() ? event_result::done : event_result::ok;
//   }
// };

// #if defined(LIB_NET_URING)

// /// @brief Specialization for uring_manager (io_uring).
// template <class NextLayer>
// class stream_transport<uring_manager, NextLayer>
//   : public datagram_transport_base<uring_manager, NextLayer> {
//   using base = datagram_transport_base<uring_manager, NextLayer>;

// public:
//   using base::base;

//   event_result handle_completion(operation op, int res) override {
//     LOG_TRACE();
//     LOG_DEBUG("Handling ", NET_ARG(op), " with ", NET_ARG(res), " on ",
//               NET_ARG2("handle", handle().id));
//     switch (op) {
//       case operation::read: {
//         const auto verdict = base::handle_read_result(res);
//         if (verdict != event_result::ok) {
//           return verdict;
//         }
//         return event_result::ok;
//       }

//       case operation::write:
//         if (res < 0) {
//           if (last_socket_error_is_temporary()) {
//             return event_result::temporary_error;
//           } else {
//             return event_result::error;
//           }
//         }
//         for (auto it = submitted_write_buffers_.begin();
//              (it < submitted_write_buffers_.end()) && (res > 0); ++it) {
//           auto& buf = *it;
//           const auto bytes_to_remove = std::min(res,
//                                                 static_cast<int>(buf.size()));
//           buf.erase(buf.begin(), buf.begin() + bytes_to_remove);
//           res -= bytes_to_remove;
//           if (buf.empty()) {
//             return_buffer(std::move(buf));
//           }
//           it = submitted_write_buffers_.erase(it);
//         }

//         prepare_next_write();
//         if (submitted_write_buffers_.empty()) {
//           return event_result::done;
//         }
//         return event_result::ok;

//       default:
//         LOG_ERROR(NET_ARG(op), " not handled by uring_stream_transport");
//         return event_result::error;
//     }
//   }

//   /// @brief Returns the buffer space available for reading.
//   /// @return A span of the available buffer space.
//   util::byte_span read_buffer() noexcept override { return
//   base::read_buffer_; }

//   /// @brief Returns the buffer with the data currently queued for writing.
//   /// @return A span of the data queued for writing.
//   std::vector<iovec> write_buffer() const noexcept override {
//     std::vector<iovec> iov;
//     iov.reserve(submitted_write_buffers_.size());
//     for (const auto& buf : submitted_write_buffers_) {
//       iov.push_back(iovec{.iov_base = buf.data(), .iov_len = buf.size()});
//     }
//     return iov;
//   }

//   void register_writing() override {
//     if ((manager_base::mask() & operation::write) == operation::none) {
//       prepare_next_write();
//     }
//     manager_base::register_writing();
//   }

// private:
//   void prepare_next_write() const {
//     // TODO: a triple buffer aproach could potentially be more performant?
//     // One for filling by the user,
//     // one that contains the partially written data,
//     // one that contains new data just being created
//     // Could save on syscalls and unnecessary copies
//     base::fetch_more_data();
//     if (!base::write_buffer_.empty()) {
//       auto buf = get_buffer();
//       std::swap(buf, base::write_buffer_);
//       submitted_write_buffers_.push_back(std::move(buf));
//     }
//   }

//   util::byte_buffer get_buffer() {
//     if (buffer_cache_.empty()) {
//       return util::byte_buffer{};
//     }
//     auto buf = std::move(buffer_cache_.front());
//     return buf; // RVO?
//   }

//   void return_buffer(util::byte_buffer&& buf) {
//     static constexpr std::size_t max_num_buffers = 10;
//     if (buffer_cache_.size() < max_num_buffers) {
//       buffer_cache_.push_back(std::move(buf));
//     }
//   }

//   // Second buffer is needed to prevent invalidating of pointers that are
//   // currently submitted in SQE's in uring
//   mutable std::vector<util::byte_buffer> submitted_write_buffers_;
//   mutable std::deque<util::byte_buffer> buffer_cache_;
// };

// #endif

// } // namespace net::detail
