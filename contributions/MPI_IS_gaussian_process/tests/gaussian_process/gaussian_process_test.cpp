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
 * Provides the test cases for the Gaussian Process functionality.
 *
 */

#include <gtest/gtest.h>
#include <vector>
#include <iostream>
#include "math_tools.h"
#include "gaussian_process.h"
#include "covariance_functions.h"

#include <fstream>

class GPTest : public ::testing::Test
{
public:
    GPTest() : random_vector_(11), location_vector_(11), hyper_parameters_(4), extra_parameters_(1)
    {
        random_vector_ << -0.1799, -1.4215, -0.2774, 2.6056, 0.6471, -0.4366, 1.3820, 0.4340, 0.8970, -0.7286, -1.7046;
        location_vector_ << 0, 0.1000, 0.2000, 0.3000, 0.4000, 0.5000, 0.6000, 0.7000, 0.8000, 0.9000, 1.0000;
        hyper_parameters_ << 1, 2, 1, 2;
        extra_parameters_ << 5;

        covariance_function_ = covariance_functions::PeriodicSquareExponential(hyper_parameters_);
        covariance_function_.setExtraParameters(extra_parameters_);

        gp_ = GP(covariance_function_);
    }
    GP gp_;
    Eigen::VectorXd random_vector_;
    Eigen::VectorXd location_vector_;
    Eigen::VectorXd hyper_parameters_;
    Eigen::VectorXd extra_parameters_;
    covariance_functions::PeriodicSquareExponential covariance_function_;
};

// This test is based on Matlab computations
TEST_F(GPTest, drawSample_prior_test)
{
    Eigen::VectorXd sample = gp_.drawSample(location_vector_, random_vector_);
    Eigen::VectorXd expected_sample(11);
    expected_sample << -1.8799, -2.2659, -2.6541, -3.0406, -3.4214, -3.7926, -4.1503, -4.4907, -4.8101, -5.1052, -5.3726;
    for (int i = 0; i < expected_sample.rows(); i++)
    {
        EXPECT_NEAR(sample(i), expected_sample(i), 2e-1);
    }
}

// This test is based on statistical expectations (mean)
TEST_F(GPTest, drawSamples_prior_mean_test)
{

    hyper_parameters_ << 1, 1, 1, 1; // use of smaller hypers needs less samples

    covariance_function_ = covariance_functions::PeriodicSquareExponential(hyper_parameters_);
    gp_ = GP(covariance_function_);

    int N = 10000; // number of samples to draw
    location_vector_ = Eigen::VectorXd(1);
    location_vector_ << 1;
    Eigen::MatrixXd sample_collection(location_vector_.rows(), N);
    sample_collection.setZero(); // just in case

    for (int i = 0; i < N; i++)
    {
        Eigen::VectorXd sample = gp_.drawSample(location_vector_);
        sample_collection.col(i) = sample;
    }
    Eigen::VectorXd sample_mean = sample_collection.rowwise().mean();

    for (int i = 0; i < sample_mean.rows(); i++)
    {
        EXPECT_NEAR(0, sample_mean(i), 1e-1);
    }
}

// This test is based on statistical expectations (covariance)
TEST_F(GPTest, drawSamples_prior_covariance_test)
{

    hyper_parameters_ << 1, 1, 1, 1; // use of smaller hypers needs less samples

    covariance_function_ = covariance_functions::PeriodicSquareExponential(hyper_parameters_);
    gp_ = GP(covariance_function_);

    int N = 20000; // number of samples to draw
    location_vector_ = Eigen::VectorXd(1);
    location_vector_ << 1;
    Eigen::MatrixXd sample_collection(location_vector_.rows(), N);
    sample_collection.setZero(); // just in case

    for (int i = 0; i < N; i++)
    {
        Eigen::VectorXd sample = gp_.drawSample(location_vector_);
        sample_collection.col(i) = sample;
    }
    Eigen::MatrixXd sample_cov = sample_collection * sample_collection.transpose() / N;

    Eigen::MatrixXd expected_cov = covariance_function_.evaluate(location_vector_, location_vector_);

    for (int i = 0; i < sample_cov.rows(); i++)
    {
        for (int k = 0; k < sample_cov.cols(); k++)
        {
            EXPECT_NEAR(expected_cov(i, k), sample_cov(i, k), 2e-1);
        }
    }
}

TEST_F(GPTest, setCovarianceFunction)
{
    Eigen::VectorXd hyperparams(6);
    hyperparams << 0.1, 15, 25, 15, 5000, 700;

    GP instance_gp;
    EXPECT_TRUE(instance_gp.setCovarianceFunction(covariance_functions::PeriodicSquareExponential(hyperparams.segment(1, 4))));

    GP instance_gp2 = GP(covariance_functions::PeriodicSquareExponential(Eigen::VectorXd::Zero(4)));
    instance_gp2.setHyperParameters(hyperparams);

    for (int i = 1; i < 5; i++) // first element is different (non set)
    {
        EXPECT_NEAR(instance_gp.getHyperParameters()[i], instance_gp2.getHyperParameters()[i], 1e-8);
    }
}

TEST_F(GPTest, setCovarianceFunction_notworking_after_inference)
{
    Eigen::VectorXd hyperparams(5);
    hyperparams << 0.1, 15, 700, 25, 5000;

    GP instance_gp;
    EXPECT_TRUE(instance_gp.setCovarianceFunction(covariance_functions::PeriodicSquareExponential(hyperparams.tail(4))));

    int N = 250;
    Eigen::VectorXd location = 400 * math_tools::generate_uniform_random_matrix_0_1(N, 1) - 200 * Eigen::MatrixXd::Ones(N, 1);

    Eigen::VectorXd output_from_converged_hyperparams = instance_gp.drawSample(location);

    instance_gp.infer(location, output_from_converged_hyperparams);
    EXPECT_FALSE(instance_gp.setCovarianceFunction(covariance_functions::PeriodicSquareExponential(hyperparams.tail(4))));
}

// to be moved to some other file
TEST_F(GPTest, periodic_covariance_function_test)
{
    covariance_functions::PeriodicSquareExponential u;
    EXPECT_EQ(u.getParameterCount(), 4);

    GP instance_gp = GP(covariance_functions::PeriodicSquareExponential());
    ASSERT_EQ(instance_gp.getHyperParameters().size(), 6);
    instance_gp.setHyperParameters(Eigen::VectorXd::Zero(6)); // should not assert
}

TEST_F(GPTest, infer_prediction_clear_test)
{
    Eigen::VectorXd data_loc(1);
    data_loc << 1;
    Eigen::VectorXd data_out(1);
    data_out << 1;
    gp_.infer(data_loc, data_out);

    Eigen::VectorXd prediction_location(2);
    prediction_location << 1, 2;

    Eigen::VectorXd prediction = gp_.predict(prediction_location);

    EXPECT_NEAR(prediction(0), 1, 1e-6);
    EXPECT_FALSE(std::abs(prediction(1) - 1) < 1e-6);

    gp_.clearData();

    prediction = gp_.predict(prediction_location);

    EXPECT_NEAR(prediction(0), 0, 1e-6);
    EXPECT_NEAR(prediction(1), 0, 1e-6);
}

TEST_F(GPTest, squareDistanceTest)
{
    Eigen::MatrixXd a(4, 3);
    Eigen::MatrixXd b(4, 5);
    Eigen::MatrixXd c(3, 4);

    a << 3, 5, 5, 4, 6, 6, 3, 2, 3, 1, 0, 3;

    b << 1, 4, 5, 6, 7, 3, 4, 5, 6, 7, 0, 2, 4, 20, 2, 2, 3, -2, -2, 2;

    c << 1, 2, 3, 4, 4, 5, 6, 7, 6, 7, 8, 9;

    Eigen::MatrixXd sqdistc(4, 4);
    Eigen::MatrixXd sqdistab(3, 5);

    // Computed by Matlab
    sqdistc << 0, 3, 12, 27, 3, 0, 3, 12, 12, 3, 0, 3, 27, 12, 3, 0;

    sqdistab << 15, 6, 15, 311, 27, 33, 14, 9, 329, 9, 35, 6, 27, 315, 7;

    // Test argument order
    EXPECT_EQ(math_tools::squareDistance(a, b), math_tools::squareDistance(b, a).transpose());

    // Test that two identical Matrices give the same result
    // (wether they are the same object or not)
    EXPECT_EQ(math_tools::squareDistance(a, Eigen::MatrixXd(a)), math_tools::squareDistance(a, a));
    EXPECT_EQ(math_tools::squareDistance(a), math_tools::squareDistance(a, a));

    // Test that the implementation gives the same result as the Matlab
    // implementation
    EXPECT_EQ(math_tools::squareDistance(c, c), sqdistc);
    EXPECT_EQ(math_tools::squareDistance(a, b), sqdistab);
}

TEST_F(GPTest, CovarianceTest2)
{
    Eigen::VectorXd hyperParams(4), extraParams(1);
    hyperParams << 1, 2, 1, 2;
    hyperParams = hyperParams.array().log();
    extraParams << 500;
    extraParams = extraParams.array().log();

    Eigen::VectorXd locations(5), X(3), Y(3);
    locations << 0, 50, 100, 150, 200;
    X << 0, 100, 200;
    Y << 1, -1, 1;

    covariance_functions::PeriodicSquareExponential covFunc(hyperParams);
    covFunc.setExtraParameters(extraParams);
    GP gp(covFunc);

    Eigen::MatrixXd kxx_matlab(5, 5);
    Eigen::MatrixXd kxX_matlab(5, 3);
    Eigen::MatrixXd kXX_matlab(3, 3);

    kxx_matlab << 8.0000, 3.3046, 2.0043, 1.0803, 0.6553, 3.3046, 8.0000, 3.3046, 2.0043, 1.0803, 2.0043, 3.3046, 8.0000,
        3.3046, 2.0043, 1.0803, 2.0043, 3.3046, 8.0000, 3.3046, 0.6553, 1.0803, 2.0043, 3.3046, 8.0000;

    kxX_matlab << 8.0000, 2.0043, 0.6553, 3.3046, 3.3046, 1.0803, 2.0043, 8.0000, 2.0043, 1.0803, 3.3046, 3.3046, 0.6553,
        2.0043, 8.0000;

    kXX_matlab << 8.0000, 2.0043, 0.6553, 2.0043, 8.0000, 2.0043, 0.6553, 2.0043, 8.0000;

    Eigen::MatrixXd kxx = covFunc.evaluate(locations, locations);
    Eigen::MatrixXd kxX = covFunc.evaluate(locations, X);
    Eigen::MatrixXd kXX = covFunc.evaluate(X, X);

    for (int col = 0; col < kxx.cols(); col++)
    {
        for (int row = 0; row < kxx.rows(); row++)
        {
            EXPECT_NEAR(kxx(row, col), kxx_matlab(row, col), 0.003);
        }
    }

    for (int col = 0; col < kxX.cols(); col++)
    {
        for (int row = 0; row < kxX.rows(); row++)
        {
            EXPECT_NEAR(kxX(row, col), kxX_matlab(row, col), 0.003);
        }
    }

    for (int col = 0; col < kXX.cols(); col++)
    {
        for (int row = 0; row < kXX.rows(); row++)
        {
            EXPECT_NEAR(kXX(row, col), kXX_matlab(row, col), 0.003);
        }
    }
}

TEST_F(GPTest, CovarianceTest3)
{
    Eigen::Matrix<double, 6, 1> hyperParams;
    hyperParams << 10, 1, 1, 1, 100, 1;
    hyperParams = hyperParams.array().log();

    Eigen::VectorXd periodLength(1);
    periodLength << std::log(80);

    Eigen::VectorXd locations(5), X(3), Y(3);
    locations << 0, 50, 100, 150, 200;
    X << 0, 100, 200;

    covariance_functions::PeriodicSquareExponential2 covFunc(hyperParams);
    covFunc.setExtraParameters(periodLength);

    Eigen::MatrixXd kxx_matlab(5, 5);
    kxx_matlab << 3.00000, 1.06389, 0.97441, 1.07075, 0.27067, 1.06389, 3.00000, 1.06389, 0.97441, 1.07075, 0.97441, 1.06389,
        3.00000, 1.06389, 0.97441, 1.07075, 0.97441, 1.06389, 3.00000, 1.06389, 0.27067, 1.07075, 0.97441, 1.06389, 3.00000;

    Eigen::MatrixXd kxX_matlab(5, 3);
    kxX_matlab << 3.00000, 0.97441, 0.27067, 1.06389, 1.06389, 1.07075, 0.97441, 3.00000, 0.97441, 1.07075, 1.06389, 1.06389,
        0.27067, 0.97441, 3.00000;

    Eigen::MatrixXd kXX_matlab(3, 3);
    kXX_matlab << 3.00000, 0.97441, 0.27067, 0.97441, 3.00000, 0.97441, 0.27067, 0.97441, 3.00000;

    Eigen::MatrixXd kxx = covFunc.evaluate(locations, locations);
    Eigen::MatrixXd kxX = covFunc.evaluate(locations, X);
    Eigen::MatrixXd kXX = covFunc.evaluate(X, X);

    for (int col = 0; col < kxx.cols(); col++)
    {
        for (int row = 0; row < kxx.rows(); row++)
        {
            EXPECT_NEAR(kxx(row, col), kxx_matlab(row, col), 0.01);
        }
    }

    for (int col = 0; col < kxX.cols(); col++)
    {
        for (int row = 0; row < kxX.rows(); row++)
        {
            EXPECT_NEAR(kxX(row, col), kxX_matlab(row, col), 0.01);
        }
    }

    for (int col = 0; col < kXX.cols(); col++)
    {
        for (int row = 0; row < kXX.rows(); row++)
        {
            EXPECT_NEAR(kXX(row, col), kXX_matlab(row, col), 0.01);
        }
    }
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
