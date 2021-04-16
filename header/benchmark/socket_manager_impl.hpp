/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 29.03.2021
 */

#pragma once

#include <sstream>

#include "benchmark/result.hpp"
#include "detail/byte_container.hpp"
#include "fwd.hpp"
#include "net/multiplexer.hpp"
#include "net/operation.hpp"
#include "net/socket.hpp"
#include "net/socket_manager.hpp"

namespace benchmark {

class socket_manager_impl : public net::socket_manager {
public:
  // -- constructors -----------------------------------------------------------

  socket_manager_impl(net::socket handle, net::multiplexer* mpx, bool mirror,
                      result_ptr result);

  ~socket_manager_impl();

  // -- event handling ---------------------------------------------------------

  bool handle_read_event() override;

  bool handle_write_event() override;

  virtual std::string me() const {
    return "socket_manager_impl";
  }

private:
  bool mirror_ = true;
  size_t byte_diff_ = 0;
  detail::byte_array<8096> buf_;
  result_ptr results_ = nullptr;
};

} // namespace benchmark
