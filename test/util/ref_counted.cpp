/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "util/ref_counted.hpp"

#include "net_test.hpp"

#include <cstddef>
#include <cstdlib>

namespace {

struct dummy_obj : public util::ref_counted {
  dummy_obj(bool& deleted) : deleted_{deleted} {}
  ~dummy_obj() override { deleted_ = true; }

private:
  bool& deleted_;
};

} // namespace

TEST(ref_counted_test, ref_counting) {
  bool deleted = false;
  auto obj = new dummy_obj(deleted);
  EXPECT_EQ(obj->ref_count(), 1);
  EXPECT_FALSE(deleted);
  obj->ref();
  EXPECT_EQ(obj->ref_count(), 2);
  EXPECT_FALSE(deleted);
  obj->ref();
  EXPECT_EQ(obj->ref_count(), 3);
  EXPECT_FALSE(deleted);
  obj->deref();
  EXPECT_EQ(obj->ref_count(), 2);
  EXPECT_FALSE(deleted);
  obj->deref();
  EXPECT_EQ(obj->ref_count(), 1);
  EXPECT_FALSE(deleted);
  obj->deref();
  EXPECT_TRUE(deleted);
}
