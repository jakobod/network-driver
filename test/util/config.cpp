/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "util/config.hpp"

#include "net_test.hpp"

#include <array>
#include <cstdio>
#include <fstream>
#include <string_view>
#include <utility>

using namespace std::string_literals;

using util::config;

namespace {

constexpr const char* key1 = "key1";
constexpr const char* key2 = "key2";
constexpr const char* key3 = "key3";
constexpr const char* key4 = "key4";

const std::array<std::pair<std::string, config::entry_type>, 4> sample_entries{
  std::make_pair(key1, "value1"s),
  std::make_pair(key2, true),
  std::make_pair(key3, 123456789ll),
  std::make_pair(key4, 1234.5678),
};

struct temp_file {
  temp_file(std::string path, std::string_view content)
    : path_{std::move(path)} {
    std::ofstream os{path_};
    os << content;
  }

  ~temp_file() { std::remove(path_.c_str()); }

  const std::string path_;
};

config create_sample_config() {
  util::config cfg;
  cfg.add_config_entry(key1, "value1"s);
  cfg.add_config_entry(key2, true);
  cfg.add_config_entry(key3, 123456789ll);
  cfg.add_config_entry(key4, 1234.5678);
  return cfg;
}

} // namespace

TEST(config, add_config_entry) {
  const util::config cfg = create_sample_config();
  const auto& dict = cfg.get_entries();
  ASSERT_EQ(dict.size(), sample_entries.size());
  for (const auto& p : sample_entries)
    EXPECT_EQ(dict.at(p.first), p.second);
}

TEST(config, get) {
  const util::config cfg = create_sample_config();

  std::string str;
  bool boolean;
  std::int64_t integer;
  double floating;

  ASSERT_NO_THROW(str = *cfg.get<std::string>(key1));
  ASSERT_NO_THROW(boolean = *cfg.get<bool>(key2));
  ASSERT_NO_THROW(integer = *cfg.get<std::int64_t>(key3));
  ASSERT_NO_THROW(floating = *cfg.get<double>(key4));

  //
  EXPECT_EQ(str, "value1"s);
  EXPECT_EQ(boolean, true);
  EXPECT_EQ(integer, 123456789ll);
  EXPECT_EQ(floating, 1234.5678);

  // Check that the error cases are handled correctly
  ASSERT_EQ(cfg.get<bool>("non_existent_key"), nullptr);
  ASSERT_EQ(cfg.get<bool>(key1), nullptr);
}

TEST(config, get_or) {
  const util::config cfg = create_sample_config();

  // Check that the contained values are returned correctly
  EXPECT_EQ(cfg.get_or(key1, "other"s), "value1"s);
  EXPECT_TRUE(cfg.get_or(key2, false));
  EXPECT_EQ(cfg.get_or(key3, 42ll), 123456789ll);
  EXPECT_EQ(cfg.get_or(key4, 42.69), 1234.5678);

  // Check the fallback cases
  EXPECT_EQ(cfg.get_or("non_existent_key", "other"s), "other"s);
  EXPECT_FALSE(cfg.get_or("non_existent_key", false));
  EXPECT_EQ(cfg.get_or("non_existent_key", 42ll), 42ll);
  EXPECT_EQ(cfg.get_or("non_existent_key", 42.69), 42.69);
}

TEST(config, has_entry) {
  const util::config cfg = create_sample_config();

  EXPECT_TRUE(cfg.has_entry<std::string>(key1));
  EXPECT_TRUE(cfg.has_entry<bool>(key2));
  EXPECT_TRUE(cfg.has_entry<std::int64_t>(key3));
  EXPECT_TRUE(cfg.has_entry<double>(key4));

  EXPECT_FALSE(cfg.has_entry<bool>("non_existent_key"));
  EXPECT_FALSE(cfg.has_entry<bool>(key1));
}

TEST(config, parse) {
  const temp_file file{"/tmp/config.cfg", R"(
    key1 = value1

    test {
      key2 = true      
      key3 = 123456789
      key4 = 42.69
    }
  )"};

  util::config cfg;
  ASSERT_NO_THROW(cfg.parse(file.path_));

  EXPECT_EQ(*cfg.get<std::string>("key1"), "value1"s);
  EXPECT_TRUE(*cfg.get<bool>("test.key2"));
  EXPECT_EQ(*cfg.get<std::int64_t>("test.key3"), 123456789ll);
  EXPECT_EQ(*cfg.get<double>("test.key4"), 42.69);
}
