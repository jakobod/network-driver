/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 03.03.2021
 */

#include "packet/payload.hpp"

namespace packet {

payload::payload(detail::byte_span data) : data_(data), payload_size_(0) {
  std::cerr << "creating ethernet header" << std::endl;
}

} // namespace packet
