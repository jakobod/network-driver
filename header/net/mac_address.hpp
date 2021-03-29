/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 02.03.2021
 */

#pragma once

#include <array>

namespace net {

static constexpr size_t mac_length = 6;

using mac_address = std::array<uint8_t, mac_length>;

} // namespace net
