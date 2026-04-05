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

/// @brief Manages variable-length payload data within network packets.
/// Provides methods to append data to a fixed-size buffer and track the current
/// payload size. Intended as a base class for protocol-specific packet types.
class payload {
public:
  // -- Constructor -----------------------------------------------------------

  /// @brief Constructs a payload handler with the given buffer space.
  /// @param data A byte span representing the buffer where payload data will be
  /// stored.
  explicit payload(detail::byte_span data);

  // -- Public API -----------------------------------------------------------

  /// @brief Returns the current amount of payload data stored.
  /// @return The size in bytes of the appended payload.
  [[nodiscard]] size_t size() const { return payload_size_; }

  /// @brief Appends data to the payload buffer.
  /// @param payload The byte span containing data to append.
  /// @return The new total payload size on success, or -1 if insufficient
  /// space.
  ssize_t append(detail::byte_span payload) {
    auto space_left = data_.size() - payload_size_;
    if (payload.size() > space_left) {
      return -1;
    }
    memcpy(data_.data(), payload.data(), payload.size());
    payload_size_ += payload.size();
    return payload_size_;
  }

private:
  detail::byte_span data_; ///< The buffer where payload is stored
  size_t payload_size_;    ///< Current size of payload data
};

} // namespace packet
