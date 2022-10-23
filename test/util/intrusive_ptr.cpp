/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 */

#include "util/intrusive_ptr.hpp"

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

TEST(intrusive_ptr_test, delete_called_on_destruction) {
  bool deleted = false;
  {
    auto ptr = util::make_intrusive<dummy_obj>(deleted);
    EXPECT_FALSE(deleted);
  }
  EXPECT_TRUE(deleted);
}

TEST(intrusive_ptr_test, delete_called_on_reset) {
  bool deleted = false;
  auto ptr = util::make_intrusive<dummy_obj>(deleted);
  EXPECT_FALSE(deleted);
  ptr.reset();
  EXPECT_TRUE(deleted);
}

TEST(intrusive_ptr_test, copy_ptr) {
  bool deleted = false;
  auto ptr = util::make_intrusive<dummy_obj>(deleted);
  auto ptr_copy = ptr;
  EXPECT_FALSE(deleted);
  EXPECT_EQ(ptr_copy->ref_count(), 2);
  EXPECT_EQ(ptr->ref_count(), 2);
  ptr.reset();
  EXPECT_EQ(ptr_copy->ref_count(), 1);
  EXPECT_FALSE(deleted);
  ptr_copy.reset();
  EXPECT_TRUE(deleted);
}

TEST(intrusive_ptr_test, move_ptr) {
  bool deleted = false;
  auto ptr = util::make_intrusive<dummy_obj>(deleted);
  auto ptr_moved = std::move(ptr);
  EXPECT_EQ(ptr.get(), nullptr);
  EXPECT_FALSE(deleted);
  EXPECT_EQ(ptr_moved->ref_count(), 1);
  ptr_moved.reset();
  EXPECT_TRUE(deleted);
}

TEST(intrusive_ptr_test, release) {
  bool deleted = false;
  auto ptr = util::make_intrusive<dummy_obj>(deleted);
  auto raw_ptr = ptr.release();
  EXPECT_EQ(ptr.get(), nullptr);
  ASSERT_FALSE(deleted);
  EXPECT_EQ(raw_ptr->ref_count(), 1);
  raw_ptr->deref();
  EXPECT_TRUE(deleted);
}
