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

#include <gtest/gtest.h>

#include <chrono>
#include <functional>
#include <iostream>
#include <utility>

using namespace std::chrono_literals;

namespace net::test {

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

template <class Pred>
inline bool poll_until(Pred predicate, net::detail::multiplexer_base& mpx,
                       bool blocking = false, std::size_t max_polls = 10) {
  std::size_t num_polls = 0;
  while (!predicate() && (num_polls++ < max_polls)) {
    if (auto err = mpx.poll_once(blocking)) {
      return false;
    }
  }
  return predicate();
}

template <class Pred>
bool wait_for(Pred predicate, std::size_t max_retries = 10,
              std::chrono::steady_clock::duration sleep_interval = 100ms) {
  std::size_t num_retries = 0;
  while (!predicate() && (num_retries++ < max_retries)) {
    std::this_thread::sleep_for(sleep_interval);
  }
  return predicate();
}

} // namespace net::test
