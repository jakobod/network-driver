/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 04.03.2021
 */

#pragma once

#include <cstring>
#include <iostream>

#include "detail/byte_container.hpp"

namespace packet {

class payload {
public:
  // -- Constructor -----------------------------------------------------------

  explicit payload(detail::byte_span data);

  [[nodiscard]] size_t size() const {
    return payload_size_;
  }

  size_t append(detail::byte_span payload) {
    auto space_left = data_.size() - payload_size_;
    if (payload.size() > space_left) {
      std::cerr << "payload too large! " << payload.size() << " vs. "
                << data_.size() << " left";
      abort();
    }
    memcpy(data_.data(), payload.data(), payload.size());
    payload_size_ += payload.size();
    return payload_size_;
  }

private:
  detail::byte_span data_;
  size_t payload_size_;
};

} // namespace packet
