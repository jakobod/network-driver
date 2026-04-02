/**
 *  @author    Jakob Otto
 *  @file      test_multiplexer.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/detail/multiplexer_base.hpp"

#include "util/error.hpp"

#include <gtest/gtest.h>

struct multiplexer_mock : public net::detail::multiplexer_base {
  void add(net::detail::manager_base_ptr, net::operation) override {}

private:
  virtual manager_map::iterator del(manager_map::iterator it) override {
    return std::next(it);
  }

public:
  void enable(net::detail::manager_base&, net::operation) override {}

  void disable(net::detail::manager_base&, net::operation, bool) override {}

  util::error poll_once(bool) override { return util::none; }

  void handle_error(const util::error& err) override {
    FAIL() << "There should be no errors! " << err << std::endl;
  }
};
