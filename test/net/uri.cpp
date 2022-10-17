/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "net/uri.hpp"

#include "util/error_or.hpp"

#include <string>
#include <vector>

#include "net_test.hpp"

using namespace net;

TEST(uri_test, parse) {
  const std::string uri_str{
    "https://127.0.0.1:21345/path/to/somewhere?query=value#fragment"};
  auto maybe_uri = net::parse_uri(uri_str);
  if (auto err = util::get_error(maybe_uri))
    FAIL() << *err;
  auto loc = std::get<net::uri>(maybe_uri);
  ASSERT_EQ("https", loc.scheme());
  const std::vector<std::string> path{"path", "to", "somewhere"};
  ASSERT_EQ(path, loc.path());
  const std::vector<std::string> queries{"query=value"};
  ASSERT_EQ(queries, loc.queries());
  const std::vector<std::string> fragments{"fragment"};
  ASSERT_EQ(fragments, loc.fragments());
}
