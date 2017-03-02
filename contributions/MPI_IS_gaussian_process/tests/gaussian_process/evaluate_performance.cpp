/*
 * Copyright 2017, Max Planck Society.
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
 */

#include "gaussian_process_guider.h"
#include "guide_performance_tools.h"

const double DefaultControlGain                   = 0.60; // control gain
const int DefaultNumMinPointsForInference         = 100; // minimal number of points for doing the inference
const double DefaultMinMove                       = 0.01;

const double DefaultGaussianNoiseHyperparameter   = 1.0; // default Gaussian measurement noise

const double DefaultLengthScaleSE0Ker             = 500.0;//900.0 // length-scale of the long-range SE-kernel
const double DefaultSignalVarianceSE0Ker          = 20.0;//5.0; // signal variance of the long-range SE-kernel
const double DefaultLengthScalePerKer             = 25.0; // length-scale of the periodic kernel
const double DefaultPeriodLengthPerKer            = 500.0; // P_p, period-length of the periodic kernel
const double DefaultSignalVariancePerKer          = 30.0;//20.0; // signal variance of the periodic kernel
const double DefaultLengthScaleSE1Ker             = 7.0; // length-scale of the short-range SE-kernel
const double DefaultSignalVarianceSE1Ker          = 10.0; // signal variance of the short range SE-kernel

const int DefaultNumMinPointsForPeriodComputation = 240; // minimal number of points for doing the period identification
const int DefaultNumPointsForApproximation        = 100; // number of points used in the GP approximation
const double DefaultPredictionGain                = 0.8; // amount of GP prediction to blend in

const bool DefaultComputePeriod                   = true;



int main(int argc, char** argv)
{
    if (argc < 13)
        return -1;

    GaussianProcessGuider* GPG;

    GaussianProcessGuider::guide_parameters parameters;
    parameters.control_gain_ = DefaultControlGain;
    parameters.min_periods_for_inference_ = DefaultNumMinPointsForInference;
    parameters.min_move_ = DefaultMinMove;
    parameters.Noise_ = DefaultGaussianNoiseHyperparameter;
    parameters.SE0KLengthScale_ = DefaultLengthScaleSE0Ker;
    parameters.SE0KSignalVariance_ = DefaultSignalVarianceSE0Ker;
    parameters.PKLengthScale_ = DefaultLengthScalePerKer;
    parameters.PKPeriodLength_ = DefaultPeriodLengthPerKer;
    parameters.PKSignalVariance_ = DefaultSignalVariancePerKer;
    parameters.SE1KLengthScale_ = DefaultLengthScaleSE1Ker;
    parameters.SE1KSignalVariance_ = DefaultSignalVarianceSE1Ker;
    parameters.min_periods_for_period_estimation_ = DefaultNumMinPointsForPeriodComputation;
    parameters.points_for_approximation_ = DefaultNumPointsForApproximation;
    parameters.prediction_gain_ = DefaultPredictionGain;
    parameters.compute_period_ = DefaultComputePeriod;

    // use parameters from command line
    parameters.control_gain_ = std::stod(argv[2]);
    parameters.min_move_ = std::stod(argv[3]);
    parameters.prediction_gain_ = std::stod(argv[4]);

    parameters.SE0KSignalVariance_ = std::stod(argv[5]);
    parameters.PKSignalVariance_ = std::stod(argv[6]);
    parameters.SE1KSignalVariance_ = std::stod(argv[7]);

    parameters.SE0KLengthScale_ = std::stod(argv[8]);
    parameters.min_points_for_period_computation_ *= std::stod(argv[9]);
    parameters.min_points_for_inference_ = std::stod(argv[10]);

    parameters.PKLengthScale_ = std::stod(argv[11]);
    parameters.SE1KLengthScale_ = std::stod(argv[12]);

    GPG = new GaussianProcessGuider(parameters);

    GAHysteresis GAH;

    std::string filename;

    filename = argv[1];
    double improvement = calculate_improvement(filename, GAH, GPG);

    std::cout << improvement << std::endl;

    return 0;
}
