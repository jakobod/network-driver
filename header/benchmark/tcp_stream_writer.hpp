/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 08.04.2021
 */

#pragma once

#include <thread>

#include "detail/error.hpp"
#include "net/tcp_stream_socket.hpp"

namespace benchmark {

class tcp_stream_writer {
public:
  tcp_stream_writer(size_t to_write);

  ~tcp_stream_writer();

  detail::error init(const std::string ip, const uint16_t port);

  void run();

  void start();

  void stop();

  void join();

private:
  bool read();

  bool write();

  detail::byte_array<4096> data_;
  net::tcp_stream_socket handle_;
  size_t to_write_;

  bool running_;
  std::thread writer_thread_;
};

} // namespace benchmark
