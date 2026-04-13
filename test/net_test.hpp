/**
 *  @author    Jakob Otto
 *  @file      net_test.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net_test_macros.hpp"

#include "net/detail/multiplexer_base.hpp"
#include "net/manager_result.hpp"

#include "net/ip/v4_endpoint.hpp"

#include "net/socket/socket.hpp"
#include "net/socket/tcp_stream_socket.hpp"
#include "net/socket/udp_datagram_socket.hpp"

#include "util/byte_span.hpp"
#include "util/error_or.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <functional>
#include <iostream>
#include <tuple>
#include <utility>

using namespace std::chrono_literals;

namespace net::test {

namespace detail {

template <std::invocable Action>
class repeater {
public:
  explicit repeater(Action action) : action_(std::move(action)) {}

  template <std::invocable Pred>
  bool until(Pred predicate, std::size_t max_repetitions = 10) {
    std::size_t num_repetitions = 0;
    while (!predicate() && (num_repetitions++ < max_repetitions)) {
      if constexpr (std::is_same_v<std::invoke_result_t<Action>, bool>) {
        if (!action_()) {
          return false;
        }
      } else {
        action_();
      }
    }
    return predicate();
  }

  bool times(std::size_t max_repetitions) {
    for (std::size_t i = 0; i < max_repetitions; ++i) {
      if constexpr (std::is_same_v<std::invoke_result_t<Action>, bool>) {
        if (!action_()) {
          return false;
        }
      } else {
        action_();
      }
    }
    return true;
  }

private:
  Action action_;
};

inline manager_result
write_all_impl(std::function<std::ptrdiff_t(util::const_byte_span)> write_func,
               util::const_byte_span buf) {
  std::size_t written = 0;
  while (written != buf.size()) {
    const auto res = write_func(buf.subspan(written));
    if (res >= 0) {
      written += res;
    } else {
      if (last_socket_error_is_temporary()) {
        std::this_thread::sleep_for(10ms);
      } else {
        return manager_result::error;
      }
    }
  }
  return manager_result::done;
}

} // namespace detail

inline manager_result write_all(stream_socket handle,
                                util::const_byte_span buf) {
  return detail::write_all_impl(
    [handle](util::const_byte_span buffer) {
      return net::write(handle, buffer);
    },
    buf);
}

inline manager_result write_all(udp_datagram_socket handle,
                                util::const_byte_span buf, ip::v4_endpoint ep) {
  return detail::write_all_impl(
    [handle, ep](util::const_byte_span buffer) {
      static constexpr std::size_t max_datagram_size = 548;
      const auto datagram_size = std::min(max_datagram_size, buffer.size());
      return net::write(handle, buffer.subspan(0, datagram_size), ep);
    },
    buf);
}

inline manager_result read_all(stream_socket handle,
                               util::byte_span receive_buffer) {
  std::size_t received = 0;
  while (received < receive_buffer.size()) {
    const auto res = net::read(handle, receive_buffer.subspan(received));
    if (res > 0) {
      received += res;
    } else {
      if (res == 0) {
        return manager_result::done;
      } else {
        if (last_socket_error_is_temporary()) {
          std::this_thread::sleep_for(10ms);
          continue;
        } else {
          return manager_result::error;
        }
      }
    }
  }
  return manager_result::ok;
}

inline std::pair<manager_result, ip::v4_endpoint>
read_all(udp_datagram_socket handle, util::byte_span receive_buffer) {
  ip::v4_endpoint last_ep;
  auto ev_result = manager_result::ok;
  std::size_t received = 0;

  while (!receive_buffer.empty()) {
    const auto [ep, res] = net::read(handle, receive_buffer.subspan(received));
    if (res > 0) {
      receive_buffer = receive_buffer.subspan(res);
      last_ep = ep;
    } else {
      if (res == 0) {
        ev_result = manager_result::done;
      } else {
        if (last_socket_error_is_temporary()) {
          std::this_thread::sleep_for(10ms);
        } else {
          return {manager_result::error, last_ep};
        }
      }
      break;
    }
  }
  return {ev_result, last_ep};
}

template <std::invocable Action>
detail::repeater<Action> invoke(Action action) {
  return detail::repeater<Action>(std::move(action));
}

template <std::invocable Pred>
inline bool poll_until(Pred predicate, net::detail::multiplexer_base& mpx,
                       bool blocking = false, std::size_t max_polls = 10) {
  return invoke([&mpx, blocking] {
           if (auto err = mpx.poll_once(blocking)) {
             return false;
           }
           return true;
         })
    .until(std::move(predicate), max_polls);
}

template <std::invocable Pred>
bool wait_for(Pred predicate, std::size_t max_retries = 10,
              std::chrono::steady_clock::duration sleep_interval = 100ms) {
  return invoke(
           [sleep_interval] { std::this_thread::sleep_for(sleep_interval); })
    .until(std::move(predicate), max_retries);
}

template <std::size_t NumBytes>
static constexpr util::byte_array<NumBytes> generate_test_data() noexcept {
  util::byte_array<NumBytes> data;
  uint8_t b = 0;
  for (auto& val : data) {
    val = std::byte{b++};
  }
  return data;
}

} // namespace net::test
