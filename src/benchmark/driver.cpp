/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 07.04.2021
 */

#include "benchmark/driver.hpp"

namespace benchmark {

driver::driver() {
  // nop
}

detail::error driver::init(const std::string host, const uint16_t port,
                           size_t byte_per_sec, size_t num_writers) {
  auto byte_per_sec_per_writer = byte_per_sec / num_writers;
  for (size_t i = 0; i < num_writers; ++i) {
    writers_.emplace_back(
      std::make_shared<tcp_stream_client>(byte_per_sec_per_writer));
    if (auto err = writers_.back()->init(host, port))
      return err;
  }
  return detail::none;
}

// -- thread functions -------------------------------------------------------

void driver::start() {
  for (const auto& writer : writers_)
    writer->start();
}

void driver::stop() {
  for (const auto& writer : writers_)
    writer->stop();
}

void driver::join() {
  for (const auto& writer : writers_)
    writer->join();
}

} // namespace benchmark
