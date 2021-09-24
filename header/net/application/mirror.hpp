/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#pragma once

#include "fwd.hpp"

#include "net/error.hpp"
#include "net/event_result.hpp"
#include "net/receive_policy.hpp"

namespace net::application {

struct mirror {
  mirror() {
    // nop
  }

  template <class Parent>
  error init(Parent& parent) {
    parent.configure_next_read(receive_policy::up_to(8096));
    return none;
  }

  template <class Parent>
  bool produce(Parent& parent) {
    if (received_.empty())
      return false;
    auto& buf = parent.write_buffer();
    {
      std::lock_guard<std::mutex> guard(lock_);
      buf.insert(buf.end(), received_.begin(), received_.end());
      received_.clear();
    }
    return true;
  }

  template <class Parent>
  bool consume(Parent& parent, util::const_byte_span data) {
    {
      std::lock_guard<std::mutex> guard(lock_);
      received_.insert(received_.end(), data.begin(), data.end());
    }
    parent.configure_next_read(receive_policy::up_to(8096));
    parent.register_writing();
    return true;
  }

  template <class Parent>
  event_result handle_timeout(Parent&, uint64_t) {
    return event_result::ok;
  }

private:
  util::byte_buffer received_;
  std::mutex lock_;
};

} // namespace net::application
