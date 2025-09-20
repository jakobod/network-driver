/**
 *  @author    Jakob Otto
 *  @file      assert.hpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#pragma once

#include <cstdlib>
#include <iostream>

namespace util {

inline void assert(std::string_view condition_string, bool verdict,
                   std::string_view file, int line, std::string_view msg = "") {
  if (!verdict) {
    std::cerr << "Assertion failed: " << msg
              << "\nExpected: " << condition_string << "\nSource: " << file
              << ", line " << line << '\n';
    std::abort();
  }
}

} // namespace util

#ifdef NET_ENABLE_ASSERTIONS
#  define ASSERT(condition, msg)                                               \
    util::assert(#condition, condition, __FILE__, __LINE__, msg);
#  ifdef DEBUG
#    define DEBUG_ONLY_ASSERT(condition, msg)                                  \
      util::assert(#condition, condition, __FILE__, __LINE__, msg);
#  else
#    define DEBUG_ONLY_ASSERT(...)
#  endif

#else
#  define ASSERT(...)
#  define DEBUG_ONLY_ASSERT(...)
#endif
