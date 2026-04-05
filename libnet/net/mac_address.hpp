/**
 *  @author    Jakob Otto
 *  @file      mac_address.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "util/byte_array.hpp"

#include <cstdint>

namespace net {

/// @brief Length of a MAC address in bytes.
static constexpr const std::size_t mac_length = 6;

/// @brief Type alias for representing a MAC (Media Access Control) address.
/// A MAC address is a 6-byte hardware address that uniquely identifies a device
/// on a local area network. Stored in network-byte-order.
using mac_address = util::byte_array<mac_length>;

} // namespace net
