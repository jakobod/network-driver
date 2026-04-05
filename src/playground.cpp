/**
 *  @author    Jakob Otto
 *  @file      playground.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "meta/concepts.hpp"

#include "util/byte_array.hpp"
#include "util/config.hpp"
#include "util/error.hpp"
#include "util/error_or.hpp"
#include "util/logger.hpp"

#include "net/multiplexer.hpp"

#include "net/socket/stream_socket.hpp"

#include "net/event_result.hpp"

#include "net/detail/event_handler.hpp"

#include <cstdlib>
#include <string>

using namespace std::string_literals;

namespace {

struct dummy_manager : public net::detail::event_handler {
  dummy_manager(net::socket handle, net::detail::multiplexer_base* mpx)
    : net::detail::event_handler(handle, mpx) {
    // nop
  }

  net::event_result handle_read_event() override {
    register_writing();
    num_bytes_ = net::read(handle<net::stream_socket>(), buf_);
    return (num_bytes_ > 0) ? net::event_result::ok : net::event_result::error;
  }

  net::event_result handle_write_event() override {
    const auto res = net::write(handle<net::stream_socket>(),
                                {buf_.data(), num_bytes_});
    if (res > 0) {
      num_bytes_ -= res;
    } else {
      LOG_ERROR(net::last_socket_error_as_string());
    }
    return (num_bytes_ == 0) ? net::event_result::done : net::event_result::ok;
  }

private:
  util::byte_array<1024> buf_;
  std::size_t num_bytes_{0};
};

} // namespace

int main(int, const char**) {
  util::config cfg;
  // try {
  //   cfg.parse("config.cfg");
  // } catch (const std::runtime_error& err) {
  //   // LOG_ERROR(err.what());
  // }
  LOG_INIT(cfg);
  auto factory = [](net::socket handle, net::detail::multiplexer_base* mpx) {
    return util::make_intrusive<dummy_manager>(handle, mpx);
  };
  auto res = net::make_multiplexer(std::move(factory), cfg);
  if ([[maybe_unused]] auto err = util::get_error(res)) {
    LOG_ERROR("Failed to create multiplexer: ", *err);
    return EXIT_FAILURE;
  }
  auto mpx = std::get<net::multiplexer_ptr>(res);
  mpx->start();

  // std::string dummy;
  // std::getline(std::cin, dummy);

  mpx->shutdown();
  mpx->join();
  return EXIT_SUCCESS;
}
