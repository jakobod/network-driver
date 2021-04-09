/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 08.04.2021
 */

#pragma once

#include <random>
#include <thread>

#include "detail/error.hpp"
#include "net/tcp_stream_socket.hpp"

namespace benchmark {

class tcp_stream_writer {
public:
  tcp_stream_writer() = default;

  tcp_stream_writer(const std::string ip, const uint16_t port, std::mt19937 mt);

  ~tcp_stream_writer();

  detail::error init();

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
  };

  // -- Private member functions -----------------------------------------------

  detail::error connect();

  detail::error reconnect();

  state read();

  state write();

  // -- members ----------------------------------------------------------------

  // Remote node identifiers
  const std::string ip_;
  const uint16_t port_;

  // read and write state
  detail::byte_array<8096> data_;
  net::tcp_stream_socket handle_;
  size_t written_;
  size_t to_write_;

  // thread state
  bool running_;
  std::thread writer_thread_;

  // Random state
  std::mt19937 mt_;
  std::uniform_int_distribution<size_t> dist_;
};

} // namespace benchmark
