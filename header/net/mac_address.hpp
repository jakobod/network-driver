/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "net/fwd.hpp"

#include <cstddef>

namespace net {

static constexpr const std::size_t mac_length = 6;

using mac_address = util::byte_array<mac_length>;

} // namespace net
