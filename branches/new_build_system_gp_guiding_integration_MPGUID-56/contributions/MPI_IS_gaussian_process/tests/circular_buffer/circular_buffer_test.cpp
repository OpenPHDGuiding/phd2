/*
 * Copyright 2014-2015, Max Planck Society.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software without 
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
 
/* Created by Stephan Wenninger <stephan.wenninger@tuebingen.mpg.de> 
 */

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

    EXPECT_EQ(buffer.getLastElement(), i);
    if (i > 1) {
      EXPECT_EQ(buffer.getSecondLastElement(), i - 1);
    }
    ++i;
  }

  for (int j = 0; j < max_size; j++) {
    EXPECT_EQ(buffer.get(j), 2 * max_size + j);
  }
}

TEST(CircularBufferTest, getTest) {
  int max_size = 10;
  CircularDoubleBuffer buffer(max_size);
  for (int i = 0; i < max_size + 1; ++i) {
    buffer.append(i);
  }
  EXPECT_EQ(buffer.getLastElement(), max_size);
  EXPECT_EQ(buffer.getSecondLastElement(), max_size - 1);
}

TEST(CircularBufferTest, clearTest) {
    int max_size = 20;
    CircularDoubleBuffer buffer(max_size);

    for (int i = 0; i < 4 * max_size; ++i) {
        buffer.append(i);
    }

    buffer.clear();
    buffer.append(3);

    EXPECT_EQ(buffer.getLastElement(), 3);

    for (int j = 1; j < max_size; ++j) {
        EXPECT_EQ(buffer.get(j), 0.0);
    }
}

TEST(CircularBufferTest, lastElementIndexTest) {
    int max_size = 6;
    CircularDoubleBuffer buffer(max_size);

    buffer.append(1);
    EXPECT_EQ(buffer.getLastElement(), 1);
    buffer.append(2);
    EXPECT_EQ(buffer.getLastElement(), 2);
    EXPECT_EQ(buffer.getSecondLastElement(), 1);
}

TEST(CircularBufferTest, getEigenVectorTest) {
  int max_size = 10;

  CircularDoubleBuffer buffer(max_size);
  buffer.append(1);
  buffer.append(2);
  buffer.append(3);
  buffer.append(4);

  Eigen::VectorXd* vec = buffer.getEigenVector();

  EXPECT_EQ(vec->size(), 4);

  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ((*vec)[i], i + 1);
  }

  buffer.append(5);
  buffer.append(6);
  buffer.append(7);
  buffer.append(8);
  buffer.append(9);
  buffer.append(10);
  buffer.append(11);
  buffer.append(12);

  vec = buffer.getEigenVector();
  EXPECT_EQ(vec->size(), 10);

  /*
   * Test if getEigenVector really only returns a pointer or if there is any
   * copying involved.
   */
  (*vec)(0) = 4;
  EXPECT_EQ((*vec)(0), buffer.get(0));
}

int main(int argc, char ** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
