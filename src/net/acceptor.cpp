/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 30.03.2021
 */

#include "net/acceptor.hpp"

#include "net/multiplexer.hpp"

namespace net {

acceptor::acceptor() {
  // nop
}

// -- properties -------------------------------------------------------------

bool acceptor::handle_read_event(multiplexer* mpx) {
  return false;
}

bool handle_write_event(multiplexer* mpx) {
  return false;
}

} // namespace net
