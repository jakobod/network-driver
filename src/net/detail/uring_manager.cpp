/**
 *  @author    Jakob Otto
 *  @file      detail/uring_manager.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released
 under
 *             the GNU GPL3 License.
 */

#if defined(LIB_NET_URING)

#  include "net/detail/uring_manager.hpp"

#  include "util/assert.hpp"
#  include "util/byte_span.hpp"

namespace net::detail {

util::const_byte_span uring_manager::write_buffer() const noexcept {
  ASSERT(false, "This is a default impl");
  return util::const_byte_span{};
}

util::byte_span uring_manager::read_buffer() noexcept {
  ASSERT(false, "This is a default impl");
  return util::byte_span{};
}

} // namespace net::detail

#endif
