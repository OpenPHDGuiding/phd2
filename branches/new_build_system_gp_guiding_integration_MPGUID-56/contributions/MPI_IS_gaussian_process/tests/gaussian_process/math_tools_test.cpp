// Copyright (c) 2014 Max Planck Society

/**@file
 * @author  Edgar Klenske <eklenske@tuebingen.mpg.de>
 *
 * @date    2014-09-09
 *
 * This file tests the math tools.
 */

#include <gtest/gtest.h>
#include "../tools/math_tools.h"

TEST(MathToolsTest, ExponentialMapTest) {
  Eigen::VectorXd mu(Eigen::VectorXd(3));
  Eigen::MatrixXd E(Eigen::MatrixXd(3, 4));
  Eigen::MatrixXd em(Eigen::MatrixXd(3, 4));

  mu << 1 , 2 , 3;

  E << 1, 2, 3, 4
    , 5, 6, 7, 8
    , 9, 10, 11, 12;


  em <<

     -0.683247804572918,        0.629108629632239,
     0.850354451463765,
     -0.557094497525197,
     -1.59711413203301,         1.14496630545333,
     1.75498451816802,
     -1.11418899505039,
     -2.51098045949311,          1.66082398127441,
     2.65961458487227,
     -1.67128349257559;


  auto result = math_tools::exp_map(mu, E);

  for (int col = 0; col < result.cols(); col++) {
    for (int row = 0; row < result.rows(); row++) {
      EXPECT_NEAR(em(row, col), result(row, col), 0.003);
    }
  }
}

TEST(MathToolsTest, RandomAnimationTest) {
  Eigen::MatrixXd ra(Eigen::MatrixXd(3, 4));
  Eigen::VectorXd x(3), t(3);
  x << 1, 2, 3;
  t << 4, 5, 6;
  ra <<
     1, 3.2659863237109,   -1, -3.26598632371091,
        2, 0.816496580927725, -2, -0.816496580927727,
        3, -1.63299316185545, -3, 1.63299316185545;

  auto result = math_tools::generate_random_sequence(4, x, t);

  for (int col = 0; col < result.cols(); col++) {
    for (int row = 0; row < result.rows(); row++) {
      EXPECT_NEAR(ra(row, col), result(row, col), 0.003);
    }
  }
}

TEST(MathToolsTest, BoxMullerTest) {
  Eigen::VectorXd vRand(10);
  vRand << 0, 0.1111, 0.2222, 0.3333, 0.4444, 0.5556, 0.6667, 0.7778, 0.8889,
        1.0000;

  Eigen::VectorXd matlab_result(10);
  matlab_result <<  -6.3769,   -1.0481,    0.3012,    1.1355,    1.2735,
                -2.3210,   -1.8154,   -1.7081,   -0.9528,   -0.0000;

  Eigen::VectorXd result = math_tools::box_muller(vRand);

  for (int col = 0; col < result.cols(); col++) {
    for (int row = 0; row < result.rows(); row++) {
      EXPECT_NEAR(matlab_result(row, col), result(row, col), 0.003);
    }
  }
}

TEST(MathToolsTest, UniformMeanTest) {
  size_t N = 200000;
  Eigen::VectorXd result(N);
  result = math_tools::generate_uniform_random_matrix_0_1(N, 1);
  ASSERT_EQ(result.rows(), N);

  double mean = result.mean();

  EXPECT_NEAR(mean, 0.5, 0.005);
}

TEST(MathToolsTest, BoxMullerMeanTest) {
  size_t N = 200000;
  Eigen::VectorXd result(N), temp(N);
  temp = math_tools::generate_uniform_random_matrix_0_1(N, 1);
  result = math_tools::box_muller(temp);
  ASSERT_EQ(result.rows(), N);

  double mean = result.mean();

  EXPECT_NEAR(mean, 0, 0.005);
}


TEST(MathToolsTest, RandnMeanTest) {
  size_t N = 200000;
  Eigen::VectorXd result(N);
  result = math_tools::generate_normal_random_matrix(N, 1);
  ASSERT_EQ(result.rows(), N);

  double mean = result.mean();

  EXPECT_NEAR(mean, 0, 0.005);
}

TEST(MathToolsTest, RandnStdTest) {
  size_t N = 200000;
  Eigen::VectorXd result(N);
  result = math_tools::generate_normal_random_matrix(N, 1);
  ASSERT_EQ(result.rows(), N);

  double mean = result.mean();

  Eigen::VectorXd temp = result.array() - mean;

  double std = temp.array().pow(2).mean();

  EXPECT_NEAR(std, 1, 0.005);
}

TEST(MathToolsTest, IsNaNTest) {
  double sqrt_of_one = std::sqrt(-1);

  EXPECT_TRUE(math_tools::isNaN(sqrt_of_one));
}

TEST(MathToolsTest, IsInfTest) {
  double log_0 = std::log(0);
  double negative_log_0 = -std::log(0);
    
  // negative infinity
  EXPECT_TRUE(math_tools::isInf(log_0));
  // positive infinity
  EXPECT_TRUE(math_tools::isInf(negative_log_0));
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

