/**
 *  @author    Jakob Otto
 *  @file      payload.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <cstring>
#include <iostream>

#include "packet/fwd.hpp"

namespace packet {

class payload {
public:
  // -- Constructor -----------------------------------------------------------

  explicit payload(detail::byte_span data);

  [[nodiscard]] size_t size() const {
    return payload_size_;
  }

  ssize_t append(detail::byte_span payload) {
    auto space_left = data_.size() - payload_size_;
    if (payload.size() > space_left)
      return -1;
    memcpy(data_.data(), payload.data(), payload.size());
    payload_size_ += payload.size();
    return payload_size_;
  }

private:
  detail::byte_span data_;
  size_t payload_size_;
};

} // namespace packet
