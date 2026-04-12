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

#include "net/socket/socket.hpp"
#include "net/socket/tcp_stream_socket.hpp"

#include "util/byte_span.hpp"
#include "util/error_or.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <functional>
#include <iostream>
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

} // namespace detail

template <class Socket>
std::pair<manager_result, std::size_t>
write(Socket handle, util::const_byte_span buf) {
  auto ev_result = manager_result::ok;
  std::size_t written = 0;

  while (written != buf.size()) {
    auto* data = buf.data() + written;
    const auto size = buf.size() - written;
    const auto res = net::write(handle, {data, size});

    if (res >= 0) {
      written += res;
    } else {
      ev_result = last_socket_error_is_temporary()
                    ? manager_result::temporary_error
                    : manager_result::error;
      break;
    }
  }

  return std::make_pair(ev_result, written);
}

template <class Socket>
std::pair<manager_result, std::size_t>
read(Socket handle, util::byte_span buf) {
  auto ev_result = manager_result::ok;
  std::size_t received = 0;

  while (received != buf.size()) {
    auto* data = buf.data() + received;
    const auto size = buf.size() - received;
    const auto res = net::read(handle, std::span{data, size});

    if (res > 0) {
      received += res;
    } else {
      if (res == 0) {
        ev_result = manager_result::done;
      } else {
        ev_result = last_socket_error_is_temporary()
                      ? manager_result::temporary_error
                      : manager_result::error;
      }
      break;
    }
  }

  return std::make_pair(ev_result, received);
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

} // namespace net::test
