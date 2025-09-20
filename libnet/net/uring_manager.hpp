/**
 *  @author    Jakob Otto
 *  @file      uring_manager.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include "net/fwd.hpp"
#include "util/fwd.hpp"

#include "net/manager.hpp"

struct io_uring_cqe;

namespace net {

class uring_manager : public manager {
public:
  using manager::manager;

  /// Destructs a socket manager object
  ~uring_manager() override = default;

  virtual util::error init(const util::config& cfg) = 0;

  virtual event_result handle_read_completion(const io_uring_cqe* cqe) = 0;

  virtual event_result handle_write_completion(const io_uring_cqe* cqe) = 0;
};

using uring_manager_ptr = util::intrusive_ptr<uring_manager>;

} // namespace net
