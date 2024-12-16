/*
 * Copyright 2014-2017, Max Planck Society.
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

/* Created by Edgar Klenske <edgar.klenske@tuebingen.mpg.de>
 *
 * This file tests the math tools.
 *
 */

#include <gtest/gtest.h>
#include "../tools/math_tools.h"

TEST(MathToolsTest, BoxMullerTest)
{
    Eigen::VectorXd vRand(10);
    vRand << 0, 0.1111, 0.2222, 0.3333, 0.4444, 0.5556, 0.6667, 0.7778, 0.8889, 1.0000;

    Eigen::VectorXd matlab_result(10);
    matlab_result << -6.3769, -1.0481, 0.3012, 1.1355, 1.2735, -2.3210, -1.8154, -1.7081, -0.9528, -0.0000;

    Eigen::VectorXd result = math_tools::box_muller(vRand);

    for (int col = 0; col < result.cols(); col++)
    {
        for (int row = 0; row < result.rows(); row++)
        {
            EXPECT_NEAR(matlab_result(row, col), result(row, col), 0.003);
        }
    }
}

TEST(MathToolsTest, UniformMeanTest)
{
    size_t N = 200000;
    Eigen::VectorXd result(N);
    result = math_tools::generate_uniform_random_matrix_0_1(N, 1);
    ASSERT_EQ(result.rows(), N);

    double mean = result.mean();

    EXPECT_NEAR(mean, 0.5, 0.005);
}

TEST(MathToolsTest, BoxMullerMeanTest)
{
    size_t N = 200000;
    Eigen::VectorXd result(N), temp(N);
    temp = math_tools::generate_uniform_random_matrix_0_1(N, 1);
    result = math_tools::box_muller(temp);
    ASSERT_EQ(result.rows(), N);

    double mean = result.mean();

    EXPECT_NEAR(mean, 0, 0.005);
}

TEST(MathToolsTest, RandnMeanTest)
{
    size_t N = 200000;
    Eigen::VectorXd result(N);
    result = math_tools::generate_normal_random_matrix(N, 1);
    ASSERT_EQ(result.rows(), N);

    double mean = result.mean();

    EXPECT_NEAR(mean, 0, 0.005);
}

TEST(MathToolsTest, RandnStdTest)
{
    size_t N = 200000;
    Eigen::VectorXd result(N);
    result = math_tools::generate_normal_random_matrix(N, 1);
    ASSERT_EQ(result.rows(), N);

    double mean = result.mean();

    Eigen::VectorXd temp = result.array() - mean;

    double std = temp.array().pow(2).mean();

    EXPECT_NEAR(std, 1, 0.005);
}

TEST(MathToolsTest, IsNaNTest)
{
    double sqrt_of_one = std::sqrt(-1);

    EXPECT_TRUE(math_tools::isNaN(sqrt_of_one));
}

TEST(MathToolsTest, IsInfTest)
{
    double log_0 = std::log(0);
    double negative_log_0 = -std::log(0);

    // negative infinity
    EXPECT_TRUE(math_tools::isInf(log_0));
    // positive infinity
    EXPECT_TRUE(math_tools::isInf(negative_log_0));
}

TEST(MathToolsTest, FFTTest)
{
    Eigen::VectorXd y(8);
    y << 0, 1, 0, 1, 0, 1, 1, 1;

    Eigen::VectorXd expected_result_real(8), expected_result_imag(8);

    expected_result_real << 5, 0, -1, 0, -3, 0, -1, 0;
    expected_result_imag << 0, 1, 0, -1, 0, 1, 0, -1;

    Eigen::FFT<double> fft;
    std::vector<double> vec_data(y.data(), y.data() + y.rows() * y.cols());
    std::vector<std::complex<double>> vec_result;
    fft.fwd(vec_result, vec_data);
    Eigen::VectorXcd result = Eigen::Map<Eigen::VectorXcd>(&vec_result[0], vec_result.size());

    Eigen::VectorXd result_real(8), result_imag(8);

    result_real = result.real();
    result_imag = result.imag();

    double eps = 1E-6;

    for (int i = 0; i < result.rows(); ++i)
    {
        EXPECT_NEAR(result_real(i), expected_result_real(i), eps);
        EXPECT_NEAR(result_imag(i), expected_result_imag(i), eps);
    }
}

TEST(MathToolsTest, SpectrumTest)
{
    Eigen::VectorXd y(8);
    y << 1, 2, 1, 2, 1, 2, 1, 2;

    Eigen::VectorXd expected_amplitudes(4), expected_frequencies(4);

    expected_amplitudes << 0, 0, 0, 16;
    expected_frequencies << 0.1250, 0.2500, 0.3750, 0.5000;

    std::pair<Eigen::VectorXd, Eigen::VectorXd> result = math_tools::compute_spectrum(y, 8);
    Eigen::VectorXd amplitudes = result.first;
    Eigen::VectorXd frequencies = result.second;

    double eps = 1E-6;

    for (int i = 0; i < expected_amplitudes.rows(); ++i)
    {
        EXPECT_NEAR(amplitudes(i), expected_amplitudes(i), eps);
        EXPECT_NEAR(frequencies(i), expected_frequencies(i), eps);
    }
}

TEST(MathToolsTest, HammingTest)
{
    Eigen::VectorXd expected_window(8);
    expected_window << 0.0800, 0.2532, 0.6424, 0.9544, 0.9544, 0.6424, 0.2532, 0.0800;

    Eigen::VectorXd window = math_tools::hamming_window(8);

    double eps = 1E-4;

    for (int i = 0; i < expected_window.rows(); ++i)
    {
        EXPECT_NEAR(window(i), expected_window(i), eps);
    }
}

TEST(MathToolsTest, StdTest)
{
    Eigen::VectorXd data(6);
    data << 1, 2, 3, 4, 5, 6;
    Eigen::ArrayXd centered = data.array() - data.array().mean();
    double matlab_result = 1.8708;

    EXPECT_NEAR(math_tools::stdandard_deviation(data), matlab_result, 1e-3);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
