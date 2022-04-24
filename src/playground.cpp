#include "net/ip/v4_endpoint.hpp"
#include "net/uri.hpp"

#include "util/error_or.hpp"
#include "util/format.hpp"

#include <iostream>

int main(int, char**) {
  std::string str{
    "https://127.0.0.1:21345/path/to/somewhere?query=value#fragment"};
  // Get the scheme of the uri removing the slashes
  auto parts = util::split(str, "://");
  auto scheme = parts.front();
  str = parts.back();
  std::cout << "scheme: " << scheme << std::endl;

  // Get fragements (if exist)
  auto fragments = util::split(str, '#');
  str = fragments.front();
  fragments.erase(fragments.begin());
  std::cout << "fragments: ";
  for (const auto& v : fragments)
    std::cout << v << ", ";
  std::cout << std::endl;

  auto queries = util::split(str, '?');
  str = queries.front();
  queries.erase(queries.begin());
  std::cout << "queries: ";
  for (const auto& v : queries)
    std::cout << v << ", ";
  std::cout << std::endl;

  // Get the paths
  auto path = util::split(str, '/');
  auto auth_str = path.front();
  path.erase(path.begin());
  std::cout << "path: ";
  for (const auto& v : path)
    std::cout << v << ", ";
  std::cout << std::endl;

  // Parse the authority
  std::cout << "auth_str: " << auth_str << std::endl;
  auto maybe_auth = net::ip::parse_v4_endpoint(auth_str);
  if (auto err = util::get_error(maybe_auth)) {
    std::cerr << "ERROR: " << err << std::endl;
    exit(-1);
  }
  auto auth = std::get<net::ip::v4_endpoint>(maybe_auth);
  net::uri loc{scheme, auth, path, queries, fragments};

  std::cout << "result: " << to_string(loc) << std::endl;
  return 0;
}
