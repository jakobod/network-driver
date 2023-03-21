/**
 *  @author    Jakob Otto
 *  @file      payload.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "packet/payload.hpp"

namespace packet {

payload::payload(detail::byte_span data) : data_(data), payload_size_(0) {
  std::cerr << "creating ethernet header" << std::endl;
}

} // namespace packet
