/**
 *  @author    Jakob Otto
 *  @file      uri.cpp
 *  @copyright Copyright 2023 Jakob Otto. All rights reserved.
 *             This file is part of the network-driver project, released under
 *             the GNU GPL3 License.
 */

#include "net/uri.hpp"

#include "util/error_or.hpp"
#include "util/exception.hpp"

#include <ranges>
#include <string>
#include <vector>

#include "net_test.hpp"

using namespace net;

TEST(uri_test, parse) {
  const std::string uri_str{
    "https://127.0.0.1:21345/path/to/somewhere?query=value#fragment"};
  ASSERT_NO_THROW(net::uri{uri_str});
  net::uri uri{uri_str};
  ASSERT_EQ(uri_str, uri.original());
  ASSERT_EQ("https", uri.scheme());

  const std::vector<std::string> path{"path", "to", "somewhere"};
  ASSERT_TRUE(std::equal(path.begin(), path.end(), uri.path().begin()));

  const std::vector<std::string> queries{"query=value"};
  ASSERT_TRUE(
    std::equal(queries.begin(), queries.end(), uri.queries().begin()));

  const std::vector<std::string> fragments{"fragment"};
  ASSERT_TRUE(
    std::equal(fragments.begin(), fragments.end(), uri.fragments().begin()));
}
