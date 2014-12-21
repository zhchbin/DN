#include "foo.h"

#include "../testing/gtest/include/gtest/gtest.h"

TEST(FooTest, Return5Test) {
  EXPECT_EQ(5, Return5());
}
