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

#include <fstream>
#include <thread>

class GuidePerformanceTest : public ::testing::Test
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

    GuidePerformanceTest(): GPG(0)
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

    ~GuidePerformanceTest()
    {
        delete GPG;
    }
};

const double GuidePerformanceTest::DefaultControlGain                   = 0.8; // control gain
const int GuidePerformanceTest::DefaultNumMinPointsForInference         = 25; // minimal number of points for doing the inference
const double GuidePerformanceTest::DefaultMinMove                       = 0.2;

const double GuidePerformanceTest::DefaultGaussianNoiseHyperparameter   = 1.0; // default Gaussian measurement noise

const double GuidePerformanceTest::DefaultLengthScaleSE0Ker             = 500.0; // length-scale of the long-range SE-kernel
const double GuidePerformanceTest::DefaultSignalVarianceSE0Ker          = 10.0; // signal variance of the long-range SE-kernel
const double GuidePerformanceTest::DefaultLengthScalePerKer             = 10.0; // length-scale of the periodic kernel
const double GuidePerformanceTest::DefaultPeriodLengthPerKer            = 500.0; // P_p, period-length of the periodic kernel
const double GuidePerformanceTest::DefaultSignalVariancePerKer          = 10.0; // signal variance of the periodic kernel
const double GuidePerformanceTest::DefaultLengthScaleSE1Ker             = 5.0; // length-scale of the short-range SE-kernel
const double GuidePerformanceTest::DefaultSignalVarianceSE1Ker          = 1.0; // signal variance of the short range SE-kernel

const int GuidePerformanceTest::DefaultNumMinPointsForPeriodComputation = 100; // minimal number of points for doing the period identification
const int GuidePerformanceTest::DefaultNumPointsForApproximation        = 100; // number of points used in the GP approximation
const double GuidePerformanceTest::DefaultPredictionGain                = 1.0; // amount of GP prediction to blend in

const bool GuidePerformanceTest::DefaultComputePeriod                   = true;

#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

class CSVRow
{
    public:
        std::string const& operator[](std::size_t index) const
        {
            return m_data[index];
        }
        std::size_t size() const
        {
            return m_data.size();
        }
        void readNextRow(std::istream& str)
        {
            std::string         line;
            std::getline(str, line);

            std::stringstream   lineStream(line);
            std::string         cell;

            m_data.clear();
            while(std::getline(lineStream, cell, ','))
            {
                m_data.push_back(cell);
            }
            // This checks for a trailing comma with no data after it.
            if (!lineStream && cell.empty())
            {
                // If there was a trailing comma then add an empty element.
                m_data.push_back("");
            }
        }
    private:
        std::vector<std::string>    m_data;
};

std::istream& operator>>(std::istream& str, CSVRow& data)
{
    data.readNextRow(str);
    return str;
}

/*
 * Replicates the behavior of the standard Hysteresis algorithm.
 */
class GAHysteresis
{
public:
    double m_hysteresis;
    double m_aggression;
    double m_minMove;
    double m_lastMove;

    GAHysteresis() : m_hysteresis(0.1), m_aggression(0.7), m_minMove(0.2), m_lastMove(0.0)
    { }

    double result(double input)
    {
        double dReturn = (1.0 - m_hysteresis) * input + m_hysteresis * m_lastMove;

        dReturn *= m_aggression;

        if (fabs(input) < m_minMove)
        {
            dReturn = 0.0;
        }

        // round to three digits
        dReturn = dReturn*1000;
        if( dReturn < 0 )
        {
            dReturn = ceil(dReturn - 0.5);
        }
        else
        {
            dReturn = floor(dReturn + 0.5);
        }
        dReturn = dReturn/1000;

        m_lastMove = dReturn;

        return dReturn;
    }
};

TEST_F(GuidePerformanceTest, performance_dataset03)
{
    GAHysteresis GAH;
    GAH.m_aggression = 0.6;
    GAH.m_hysteresis = 0.3;
    GAH.m_minMove = 0.1;

    std::string filename("dataset03.csv");
    std::ifstream file(filename);

    int i = 0;
    CSVRow row;
    while(file >> row)
    {
        // ignore special lines
        if (row[0][0] == 'F' || row.size() < 18 )
        {
            continue;
        }
        else
        {
            ++i;
        }
    }

    size_t N = i;
    i = -1;

    // initialize the different vectors needed for the GP
    Eigen::VectorXd times(N);
    Eigen::VectorXd measurements(N);
    Eigen::VectorXd controls(N);
    Eigen::VectorXd SNRs(N);
    Eigen::VectorXd sum_controls(N);
    Eigen::VectorXd gear_error(N);
    double sum_control = 0;

    file.close();
    file.clear();
    file.open(filename);
    while(file >> row)
    {
        // ignore special lines
        if (row[0][0] == 'F' || row.size() < 18 )
        {
            continue;
        }
        else
        {
            ++i;
        }
        times(i) = std::stod(row[1]);
        measurements(i) = std::stod(row[5]);
        controls(i) = std::stod(row[7]);
        SNRs(i) = std::stod(row[16]);
        sum_control += controls(i); // sum over the control signals
        sum_controls(i) = sum_control; // store current accumulated control signal
    }

    gear_error = sum_controls + measurements;

    int hysteresis_mismatch = 0;
    double hysteresis_control = 0.0;
    double hysteresis_error = 0.0;
    double hysteresis_state = measurements(0);

    double gp_guider_control = 0.0;
    double gp_guider_state = measurements(0);

    Eigen::ArrayXd gp_guider_states(times.size()-1);

    for (i = 0; i < times.size()-1; ++i)
    {
        hysteresis_control = GAH.result(hysteresis_state);

        // this is a simple telescope "simulator"
        hysteresis_state = hysteresis_state + (measurements(i+1) - (measurements(i) - controls(i))) - hysteresis_control;

        // check consistency of simulator and hysteresis algorithm
        EXPECT_NEAR(hysteresis_state, measurements(i+1), 5e-2);

        GPG->reset();
        for (int j = 0; j < i; ++j)
        {
            GPG->inject_data_point(times(j), measurements(j), SNRs(j), controls(j));
        }
        gp_guider_control = GPG->result(gp_guider_state, SNRs(i), 3.0);
        gp_guider_state = gp_guider_state + (measurements(i+1) - (measurements(i) - controls(i))) - gp_guider_control;

        gp_guider_states(i) = gp_guider_state;
    }

    std::cout << "GPGuider Performance: " <<
        std::sqrt(gp_guider_states.pow(2).sum()/gp_guider_states.size()) <<
        " | " << std::sqrt(measurements.array().pow(2).sum()/measurements.size()) << std::endl;
    EXPECT_LT(std::sqrt(gp_guider_states.pow(2).sum()/gp_guider_states.size()),
              std::sqrt(measurements.array().pow(2).sum()/measurements.size()));
}


int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
