/**
 *  @author    Jakob Otto
 *  @file      byte_buffer.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <cstddef>
#include <vector>

namespace util {

using byte_buffer = std::vector<std::byte>;

} // namespace util
