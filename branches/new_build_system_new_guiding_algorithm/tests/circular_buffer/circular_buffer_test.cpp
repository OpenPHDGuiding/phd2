#include <gtest/gtest.h>
#include "circular_buffer.h"

TEST(CircularBufferTest, noDataPointsDeletedTest) {
  int max_size = 5;
  CircularDoubleBuffer buffer(max_size);

  int i = 0;
  while (i  < max_size) {
    buffer.append(i);
    i++;
  }

  for (int j = 0; j < max_size; ++j) {
    EXPECT_EQ(buffer.get(j), j);
  }
}

TEST(CircularBufferTest, exceedMaxSizeby1Test) {
  int max_size = 6;
  CircularDoubleBuffer buffer(max_size);

  int i = 0;
  while (i < max_size + 1) {
    buffer.append(i);
    ++i;
  }

  EXPECT_EQ(buffer.get(0), max_size);
  for (int j = 1; j < max_size; ++j) {
    EXPECT_EQ(buffer.get(j), j);
  }
}

TEST(CircularBufferTest, overflow3TimesTest) {
  int max_size = 10;
  CircularDoubleBuffer buffer(max_size);

  int i = 0;
  while (i < 3 * max_size) {
    buffer.append(i);
    ++i;
  }

  for (int j = 0; j < max_size; j++) {
    EXPECT_EQ(buffer.get(j), 2 * max_size + j);
  }
}

TEST(CircularBufferTest, clearTest) {
    int max_size = 20;
    CircularDoubleBuffer buffer(max_size);

    for (int i = 0; i < 4 * max_size; ++i) {
        buffer.append(i);
    }

    buffer.clear();
    buffer.append(3);

    EXPECT_EQ(buffer.get(buffer.lastElementIndex()), 3);

    for (int j = 1; j < max_size; ++j) {
        EXPECT_EQ(buffer.get(j), 0.0);
    }
}

TEST(CircularBufferTest, lastElementIndexTest) {
    int max_size = 6;
    CircularDoubleBuffer buffer(max_size);

    buffer.append(1);
    EXPECT_EQ(buffer.get(buffer.lastElementIndex()), 1);
    buffer.append(2);
    EXPECT_EQ(buffer.get(buffer.lastElementIndex()), 2);
    EXPECT_EQ(buffer.get(buffer.lastElementIndex() - 1), 1);
}

int main(int argc, char ** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}