/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 09.04.2021
 */

#pragma once

#include <random>
#include <vector>

#include "benchmark/tcp_stream_writer.hpp"
#include "detail/error.hpp"
#include "fwd.hpp"

namespace benchmark {

class driver {
public:
  // -- constructors -----------------------------------------------------------

  driver();

  detail::error init(const std::string host, const uint16_t port,
                     size_t num_writers);

  void run();

  // -- thread functions -------------------------------------------------------

  void start();

  void stop();

  void join();

private:
  std::random_device rd_;
  std::vector<tcp_stream_writer_ptr> writers_;
};

} // namespace benchmark
