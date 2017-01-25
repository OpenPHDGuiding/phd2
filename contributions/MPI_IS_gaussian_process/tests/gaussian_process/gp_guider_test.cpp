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

/* Created by Edgar Klenske <edgar.klenske@tuebingen.mpg.de>
 *
 * Provides the test cases for the Gaussian Process functionality.
 *
 */

#include <gtest/gtest.h>
#include <vector>
#include <iostream>
#include "gaussian_process_guider.h"

#include <fstream>

class GPGTest : public ::testing::Test
{
public:
    static const double DefaultControlGain; // control gain
    static const int    DefaultNumMinPointsForInference; // minimal number of points for doing the inference
    static const double DefaultMinMove;

    static const double DefaultGaussianNoiseHyperparameter; // default Gaussian measurement noise

    static const double DefaultLengthScaleSE0Ker; // length-scale of the long-range SE-kernel
    static const double DefaultSignalVarianceSE0Ker; // signal variance of the long-range SE-kernel
    static const double DefaultLengthScalePerKer; // length-scale of the periodic kernel
    static const double DefaultPeriodLengthPerKer; // P_p, period-length of the periodic kernel
    static const double DefaultSignalVariancePerKer; // signal variance of the periodic kernel
    static const double DefaultLengthScaleSE1Ker; // length-scale of the short-range SE-kernel
    static const double DefaultSignalVarianceSE1Ker; // signal variance of the short range SE-kernel

    static const int    DefaultNumMinPointsForPeriodComputation; // minimal number of points for doing the period identification
    static const int    DefaultNumPointsForApproximation; // number of points used in the GP approximation
    static const double DefaultPredictionGain; // amount of GP prediction to blend in

    static const bool   DefaultComputePeriod;

    GaussianProcessGuider* GPG;

    GPGTest(): GPG(0)
    {
        GaussianProcessGuider::guide_parameters parameters;
        parameters.control_gain_ = DefaultControlGain;
        parameters.min_points_for_inference_ = DefaultNumMinPointsForInference;
        parameters.min_move_ = DefaultMinMove;
        parameters.Noise_ = DefaultGaussianNoiseHyperparameter;
        parameters.SE0KLengthScale_ = DefaultLengthScaleSE0Ker;
        parameters.SE0KSignalVariance_ = DefaultSignalVarianceSE0Ker;
        parameters.PKLengthScale_ = DefaultLengthScalePerKer;
        parameters.PKPeriodLength_ = DefaultPeriodLengthPerKer;
        parameters.PKSignalVariance_ = DefaultSignalVariancePerKer;
        parameters.SE1KLengthScale_ = DefaultLengthScaleSE1Ker;
        parameters.SE1KSignalVariance_ = DefaultSignalVarianceSE1Ker;
        parameters.min_points_for_period_computation_ = DefaultNumMinPointsForPeriodComputation;
        parameters.points_for_approximation_ = DefaultNumPointsForApproximation;
        parameters.prediction_gain_ = DefaultPredictionGain;
        parameters.compute_period_ = DefaultComputePeriod;

        GPG = new GaussianProcessGuider(parameters);
    }

    ~GPGTest()
    {
        delete GPG;
    }
};

const double GPGTest::DefaultControlGain                   = 0.8; // control gain
const int GPGTest::DefaultNumMinPointsForInference      = 25; // minimal number of points for doing the inference
const double GPGTest::DefaultMinMove                       = 0.2;

const double GPGTest::DefaultGaussianNoiseHyperparameter   = 1.0; // default Gaussian measurement noise

const double GPGTest::DefaultLengthScaleSE0Ker             = 500.0; // length-scale of the long-range SE-kernel
const double GPGTest::DefaultSignalVarianceSE0Ker          = 10.0; // signal variance of the long-range SE-kernel
const double GPGTest::DefaultLengthScalePerKer             = 0.5; // length-scale of the periodic kernel
const double GPGTest::DefaultPeriodLengthPerKer            = 500; // P_p, period-length of the periodic kernel
const double GPGTest::DefaultSignalVariancePerKer          = 10.0; // signal variance of the periodic kernel
const double GPGTest::DefaultLengthScaleSE1Ker             = 5.0; // length-scale of the short-range SE-kernel
const double GPGTest::DefaultSignalVarianceSE1Ker          = 1.0; // signal variance of the short range SE-kernel

const int GPGTest::DefaultNumMinPointsForPeriodComputation = 100; // minimal number of points for doing the period identification
const int GPGTest::DefaultNumPointsForApproximation        = 100; // number of points used in the GP approximation
const double GPGTest::DefaultPredictionGain                  = 1.0; // amount of GP prediction to blend in

const bool GPGTest::DefaultComputePeriod                 = true;

TEST_F(GPGTest, simple_result_test)
{
    double result = 0.0;

    // for an empty dataset, deduceResult should return zero
    result = GPG->deduceResult(3.0);
    EXPECT_NEAR(result, 0, 1e-6);

    // for an empty dataset, result is equivalent to a P-controller
    result = GPG->result(1.0, 2.0, 3.0);
    EXPECT_NEAR(result, 0.8, 1e-6); // result should be measurement x control gain
}

TEST_F(GPGTest, parameters_test)
{
    EXPECT_NEAR(GPG->GetControlGain(), DefaultControlGain, 1e-6);
    EXPECT_NEAR(GPG->GetNumPointsInference(), DefaultNumMinPointsForInference, 1e-6);
    EXPECT_NEAR(GPG->GetMinMove(), DefaultMinMove, 1e-6);

    std::vector<double> parameters = GPG->GetGPHyperparameters();
    EXPECT_NEAR(parameters[1], DefaultLengthScaleSE0Ker, 1e-6);
    EXPECT_NEAR(parameters[2], DefaultSignalVarianceSE0Ker, 1e-6);
    EXPECT_NEAR(parameters[3], DefaultLengthScalePerKer, 1e-6);
    EXPECT_NEAR(parameters[4], DefaultPeriodLengthPerKer, 1e-6);
    EXPECT_NEAR(parameters[5], DefaultSignalVariancePerKer, 1e-6);
    EXPECT_NEAR(parameters[6], DefaultLengthScaleSE1Ker, 1e-6);
    EXPECT_NEAR(parameters[7], DefaultSignalVarianceSE1Ker, 1e-6);

    EXPECT_NEAR(GPG->GetNumPointsPeriodComputation(), DefaultNumMinPointsForPeriodComputation, 1e-6);
    EXPECT_NEAR(GPG->GetNumPointsForApproximation(), DefaultNumPointsForApproximation, 1e-6);
    EXPECT_NEAR(GPG->GetPredictionGain(), DefaultPredictionGain, 1e-6);
    EXPECT_NEAR(GPG->GetBoolComputePeriod(), DefaultComputePeriod, 1e-6);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
