/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "util/byte_array.hpp"

#include <cstdint>

namespace net {

static constexpr const std::size_t mac_length = 6;

using mac_address = util::byte_array<mac_length>;

} // namespace net
