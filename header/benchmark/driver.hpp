/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 09.04.2021
 */

#pragma once

#include <vector>

#include "benchmark/tcp_stream_writer.hpp"s

namespace benchmark {

class driver {
public:
  // -- constructors -----------------------------------------------------------

  driver(size_t num_writers);

private:
  std::vector<tcp_stream_writer> writers;
};

} // namespace benchmark
