/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 08.04.2021
 */

#pragma once

#include <random>
#include <sys/epoll.h>
#include <thread>

#include "detail/error.hpp"
#include "net/operation.hpp"
#include "net/tcp_stream_socket.hpp"

namespace benchmark {

class tcp_stream_client {
  static constexpr size_t max_epoll_events = 2;
  using pollset = std::array<epoll_event, max_epoll_events>;
  using epoll_fd = int;

public:
  tcp_stream_client() = default;

  tcp_stream_client(const std::string ip, const uint16_t port, std::mt19937 mt);

  ~tcp_stream_client();

  detail::error init();

  // -- thread functions -------------------------------------------------------

  void run();

  void start();

  void stop();

  void join();

private:
  // -- read/write results -----------------------------------------------------

  enum class state {
    go_on = 0,
    done,
    error,
    disconnected,
  };

  // -- Private member functions -----------------------------------------------

  detail::error connect();

  detail::error reconnect();

  state read();

  state write();

  // -- epoll management -------------------------------------------------------

  void add(net::socket handle);

  void del(net::socket handle);

  void mod(int fd, int op, net::operation events);

  // -- members ----------------------------------------------------------------

  // Remote node identifiers
  const std::string ip_;
  const uint16_t port_;
  net::tcp_stream_socket handle_;

  // epoll variables
  epoll_fd epoll_fd_;
  pollset pollset_;

  // read and write state
  detail::byte_array<8096> data_;
  size_t written_;
  size_t received_;
  size_t write_goal_;

  // thread state
  bool running_;
  std::thread writer_thread_;

  // Random state
  std::mt19937 mt_;
  std::uniform_int_distribution<size_t> dist_;
};

} // namespace benchmark
