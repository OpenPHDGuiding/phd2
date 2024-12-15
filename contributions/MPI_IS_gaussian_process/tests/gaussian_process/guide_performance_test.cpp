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
#include "gaussian_process_guider.h"
#include "guide_performance_tools.h"

#include <fstream>
#include <thread>

class GuidePerformanceTest : public ::testing::Test
{
public:
    static const double DefaultControlGain; // control gain
    static const double DefaultPeriodLengthsInference; // minimal number of points for doing the inference
    static const double DefaultMinMove;

    static const double DefaultLengthScaleSE0Ker; // length-scale of the long-range SE-kernel
    static const double DefaultSignalVarianceSE0Ker; // signal variance of the long-range SE-kernel
    static const double DefaultLengthScalePerKer; // length-scale of the periodic kernel
    static const double DefaultPeriodLengthPerKer; // P_p, period-length of the periodic kernel
    static const double DefaultSignalVariancePerKer; // signal variance of the periodic kernel
    static const double DefaultLengthScaleSE1Ker; // length-scale of the short-range SE-kernel
    static const double DefaultSignalVarianceSE1Ker; // signal variance of the short range SE-kernel

    static const double DefaultPeriodLengthsPeriodEstimation; // minimal number of points for doing the period identification
    static const int DefaultNumPointsForApproximation; // number of points used in the GP approximation
    static const double DefaultPredictionGain; // amount of GP prediction to blend in

    static const bool DefaultComputePeriod;

    GaussianProcessGuider *GPG;
    GAHysteresis GAH;
    std::string filename;
    double improvement;

    GuidePerformanceTest() : GPG(0), improvement(0.0)
    {
        GaussianProcessGuider::guide_parameters parameters;
        parameters.control_gain_ = DefaultControlGain;
        parameters.min_periods_for_inference_ = DefaultPeriodLengthsInference;
        parameters.min_move_ = DefaultMinMove;
        parameters.SE0KLengthScale_ = DefaultLengthScaleSE0Ker;
        parameters.SE0KSignalVariance_ = DefaultSignalVarianceSE0Ker;
        parameters.PKLengthScale_ = DefaultLengthScalePerKer;
        parameters.PKPeriodLength_ = DefaultPeriodLengthPerKer;
        parameters.PKSignalVariance_ = DefaultSignalVariancePerKer;
        parameters.SE1KLengthScale_ = DefaultLengthScaleSE1Ker;
        parameters.SE1KSignalVariance_ = DefaultSignalVarianceSE1Ker;
        parameters.min_periods_for_period_estimation_ = DefaultPeriodLengthsPeriodEstimation;
        parameters.points_for_approximation_ = DefaultNumPointsForApproximation;
        parameters.prediction_gain_ = DefaultPredictionGain;
        parameters.compute_period_ = DefaultComputePeriod;

        GPG = new GaussianProcessGuider(parameters);
    }

    ~GuidePerformanceTest() { delete GPG; }
};

const double GuidePerformanceTest::DefaultControlGain = 0.7; // control gain
const double GuidePerformanceTest::DefaultPeriodLengthsInference = 2.0; // period lengths until inference
const double GuidePerformanceTest::DefaultMinMove = 0.2; // minimal move

const double GuidePerformanceTest::DefaultLengthScaleSE0Ker = 700.0; // length-scale of the long-range SE-kernel
const double GuidePerformanceTest::DefaultSignalVarianceSE0Ker = 20.0; // signal variance of the long-range SE-kernel
const double GuidePerformanceTest::DefaultLengthScalePerKer = 10.0; // length-scale of the periodic kernel
const double GuidePerformanceTest::DefaultPeriodLengthPerKer = 200.0; // P_p, period-length of the periodic kernel
const double GuidePerformanceTest::DefaultSignalVariancePerKer = 20.0; // signal variance of the periodic kernel
const double GuidePerformanceTest::DefaultLengthScaleSE1Ker = 25.0; // length-scale of the short-range SE-kernel
const double GuidePerformanceTest::DefaultSignalVarianceSE1Ker = 10.0; // signal variance of the short range SE-kernel

const double GuidePerformanceTest::DefaultPeriodLengthsPeriodEstimation = 2.0; // period lengths until FFT
const int GuidePerformanceTest::DefaultNumPointsForApproximation = 100; // number of points used in the GP approximation
const double GuidePerformanceTest::DefaultPredictionGain = 0.5; // amount of GP prediction to blend in

const bool GuidePerformanceTest::DefaultComputePeriod = true;

TEST_F(GuidePerformanceTest, performance_dataset01)
{
    filename = "performance_dataset01.txt";
    improvement = calculate_improvement(filename, GAH, GPG);
    std::cout << "Improvement of GPGuiding over Hysteresis: " << 100 * improvement << "%" << std::endl;
    EXPECT_GT(improvement, 0);
}

TEST_F(GuidePerformanceTest, performance_dataset02)
{
    filename = "performance_dataset02.txt";
    improvement = calculate_improvement(filename, GAH, GPG);
    std::cout << "Improvement of GPGuiding over Hysteresis: " << 100 * improvement << "%" << std::endl;
    EXPECT_GT(improvement, 0);
}

TEST_F(GuidePerformanceTest, performance_dataset03)
{
    filename = "performance_dataset03.txt";
    improvement = calculate_improvement(filename, GAH, GPG);
    std::cout << "Improvement of GPGuiding over Hysteresis: " << 100 * improvement << "%" << std::endl;
    EXPECT_GT(improvement, 0);
}

TEST_F(GuidePerformanceTest, performance_dataset04)
{
    filename = "performance_dataset04.txt";
    improvement = calculate_improvement(filename, GAH, GPG);
    std::cout << "Improvement of GPGuiding over Hysteresis: " << 100 * improvement << "%" << std::endl;
    EXPECT_GT(improvement, 0);
}

TEST_F(GuidePerformanceTest, performance_dataset05)
{
    filename = "performance_dataset05.txt";
    improvement = calculate_improvement(filename, GAH, GPG);
    std::cout << "Improvement of GPGuiding over Hysteresis: " << 100 * improvement << "%" << std::endl;
    EXPECT_GT(improvement, 0);
}

TEST_F(GuidePerformanceTest, performance_dataset06)
{
    filename = "performance_dataset06.txt";
    improvement = calculate_improvement(filename, GAH, GPG);
    std::cout << "Improvement of GPGuiding over Hysteresis: " << 100 * improvement << "%" << std::endl;
    EXPECT_GT(improvement, 0);
}

TEST_F(GuidePerformanceTest, performance_dataset07)
{
    filename = "performance_dataset07.txt";
    improvement = calculate_improvement(filename, GAH, GPG);
    std::cout << "Improvement of GPGuiding over Hysteresis: " << 100 * improvement << "%" << std::endl;
    EXPECT_GT(improvement, 0);
}

TEST_F(GuidePerformanceTest, performance_dataset08)
{
    filename = "performance_dataset08.txt";
    improvement = calculate_improvement(filename, GAH, GPG);
    std::cout << "Improvement of GPGuiding over Hysteresis: " << 100 * improvement << "%" << std::endl;
    EXPECT_GT(improvement, 0);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
