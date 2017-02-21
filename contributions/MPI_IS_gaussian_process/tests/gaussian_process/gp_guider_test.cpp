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
const int GPGTest::DefaultNumMinPointsForInference         = 25; // minimal number of points for doing the inference
const double GPGTest::DefaultMinMove                       = 0.2;

const double GPGTest::DefaultGaussianNoiseHyperparameter   = 1.0; // default Gaussian measurement noise

const double GPGTest::DefaultLengthScaleSE0Ker             = 500.0; // length-scale of the long-range SE-kernel
const double GPGTest::DefaultSignalVarianceSE0Ker          = 10.0; // signal variance of the long-range SE-kernel
const double GPGTest::DefaultLengthScalePerKer             = 10.0; // length-scale of the periodic kernel
const double GPGTest::DefaultPeriodLengthPerKer            = 500.0; // P_p, period-length of the periodic kernel
const double GPGTest::DefaultSignalVariancePerKer          = 10.0; // signal variance of the periodic kernel
const double GPGTest::DefaultLengthScaleSE1Ker             = 5.0; // length-scale of the short-range SE-kernel
const double GPGTest::DefaultSignalVarianceSE1Ker          = 1.0; // signal variance of the short range SE-kernel

const int GPGTest::DefaultNumMinPointsForPeriodComputation = 100; // minimal number of points for doing the period identification
const int GPGTest::DefaultNumPointsForApproximation        = 100; // number of points used in the GP approximation
const double GPGTest::DefaultPredictionGain                = 1.0; // amount of GP prediction to blend in

const bool GPGTest::DefaultComputePeriod                   = true;

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

TEST_F(GPGTest, period_identification_test)
{
    // first: prepare a nice GP with a sine wave
    double period_length = 300;
    double max_time = 10*period_length;
    int resolution = 500;
    Eigen::VectorXd timestamps = Eigen::VectorXd::LinSpaced(resolution + 1, 0, max_time);
    Eigen::VectorXd measurements = 50*(timestamps.array()*2*M_PI/period_length).sin();
    Eigen::VectorXd controls = 0*measurements;
    Eigen::VectorXd SNRs = 100*Eigen::VectorXd::Ones(resolution + 1);

    // feed data to the GPGuider
    for (int i = 0; i < timestamps.size(); ++i)
    {
        GPG->inject_data_point(timestamps[i], measurements[i], SNRs[i], controls[i]);
    }
    GPG->result(0.15, 2.0, 3.0);

    EXPECT_NEAR(GPG->GetGPHyperparameters()[7], period_length, 1);
}

TEST_F(GPGTest, min_move_test)
{
    // simple min-moves (without GP data)
    EXPECT_NEAR(GPG->result(0.15, 2.0, 3.0), 0, 1e-6);
    GPG->reset();
    EXPECT_NEAR(GPG->result(0.25, 2.0, 3.0), 0.25*0.8, 1e-6);
    GPG->reset();
    EXPECT_NEAR(GPG->result(-0.15, 2.0, 3.0), 0, 1e-6);
    GPG->reset();
    EXPECT_NEAR(GPG->result(-0.25, 2.0, 3.0), -0.25*0.8, 1e-6);
    GPG->reset();
}

TEST_F(GPGTest, gp_prediction_test)
{

    // first: prepare a nice GP with a sine wave
    double period_length = 300;
    double max_time = 5*period_length;
    int resolution = 600;
    double prediction_length = 3.0;
    Eigen::VectorXd locations(2);
    Eigen::VectorXd predictions(2);
    Eigen::VectorXd timestamps = Eigen::VectorXd::LinSpaced(resolution + 1, 0, max_time);
    Eigen::VectorXd measurements = 50*(timestamps.array()*2*M_PI/period_length).sin();
    Eigen::VectorXd controls = 0*measurements;
    Eigen::VectorXd SNRs = 100*Eigen::VectorXd::Ones(resolution + 1);

    // feed data to the GPGuider
    for (int i = 0; i < timestamps.size(); ++i)
    {
        GPG->inject_data_point(timestamps[i], measurements[i], SNRs[i], controls[i]);
    }
    locations << max_time, max_time + prediction_length;
    predictions = 50*(locations.array()*2*M_PI/period_length).sin();
    // the first case is with an error smaller than min_move_
    EXPECT_NEAR(GPG->result(0.15, 2.0, prediction_length, max_time), predictions[1]-predictions[0], 2e-1);
    GPG->reset();

    // feed data to the GPGuider
    for (int i = 0; i < timestamps.size(); ++i)
    {
        GPG->inject_data_point(timestamps[i], measurements[i], SNRs[i], controls[i]);
    }
    // the first case is with an error larger than min_move_
    EXPECT_NEAR(GPG->result(0.25, 2.0, prediction_length, max_time), 0.25*0.8+predictions[1]-predictions[0], 2e-1);
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
    EXPECT_NEAR(parameters[4], DefaultSignalVariancePerKer, 1e-6);
    EXPECT_NEAR(parameters[5], DefaultLengthScaleSE1Ker, 1e-6);
    EXPECT_NEAR(parameters[6], DefaultSignalVarianceSE1Ker, 1e-6);
    EXPECT_NEAR(parameters[7], DefaultPeriodLengthPerKer, 1e-6);

    EXPECT_NEAR(GPG->GetNumPointsPeriodComputation(), DefaultNumMinPointsForPeriodComputation, 1e-6);
    EXPECT_NEAR(GPG->GetNumPointsForApproximation(), DefaultNumPointsForApproximation, 1e-6);
    EXPECT_NEAR(GPG->GetPredictionGain(), DefaultPredictionGain, 1e-6);
    EXPECT_NEAR(GPG->GetBoolComputePeriod(), DefaultComputePeriod, 1e-6);
}

TEST_F(GPGTest, timer_test)
{
    double result = 0;
    int wait = 500;

    GPG->result(1.0, 2.0, 3.0);
    std::this_thread::sleep_for(std::chrono::milliseconds(wait));

    auto time_start = std::chrono::system_clock::now();
    GPG->result(1.0, 2.0, 3.0);
    double first_time = GPG->get_second_last_point().timestamp;
    std::this_thread::sleep_for(std::chrono::milliseconds(wait));
    auto time_end = std::chrono::system_clock::now();
    GPG->result(1.0, 2.0, 3.0);
    double second_time = GPG->get_second_last_point().timestamp;

    EXPECT_NEAR(second_time - first_time, std::chrono::duration<double>(time_end - time_start).count(), 1e-1);
}

TEST_F(GPGTest, gp_projection_test)
{   // this test should fail when output projections are disabled and should pass when they are enabled

    // first: prepare a nice GP with a sine wave
    double period_length = 300;
    double max_time = 5*period_length;
    int resolution = 600;
    double prediction_length = 3.0;
    Eigen::VectorXd locations(2);
    Eigen::VectorXd predictions(2);
    Eigen::VectorXd timestamps = Eigen::VectorXd::LinSpaced(resolution + 1, 0, max_time);
    Eigen::VectorXd measurements = 50*(timestamps.array()*2*M_PI/period_length).sin();
    Eigen::VectorXd controls = 0*measurements;
    Eigen::VectorXd SNRs = 100*Eigen::VectorXd::Ones(resolution + 1);

    Eigen::VectorXd sine_noise = 5*(timestamps.array()*2*M_PI/26).sin(); // smaller "disturbance" to add

    measurements = measurements + sine_noise;

    // feed data to the GPGuider
    for (int i = 0; i < timestamps.size(); ++i)
    {
        GPG->inject_data_point(timestamps[i], measurements[i], SNRs[i], controls[i]);
    }
    locations << max_time, max_time + prediction_length;
    predictions = 50*(locations.array()*2*M_PI/period_length).sin();
    // the first case is with an error smaller than min_move_
    EXPECT_NEAR(GPG->result(0.0, 2.0, prediction_length, max_time), predictions[1]-predictions[0], 3e-1);
    GPG->reset();
}

TEST_F(GPGTest, linear_drift_identification_test)
{   // when predicting one period length ahead, only linear drift should show

    // first: prepare a nice GP with a sine wave
    double period_length = 300;
    double max_time = 3*period_length;
    int resolution = 300;
    double prediction_length = period_length; // necessary to only see the drift
    Eigen::VectorXd locations(2);
    Eigen::VectorXd predictions(2);
    Eigen::VectorXd timestamps = Eigen::VectorXd::LinSpaced(resolution + 1, 0, max_time);
    Eigen::VectorXd measurements = 0*timestamps;
    Eigen::VectorXd sine_data = 50*(timestamps.array()*2*M_PI/period_length).sin();
    Eigen::VectorXd drift = 0.25*timestamps; // drift to add
    Eigen::VectorXd gear_function = sine_data + drift;
    Eigen::VectorXd controls(timestamps.size());
    controls << gear_function.tail(gear_function.size()-1) - gear_function.head(gear_function.size()-1), 0;
    Eigen::VectorXd SNRs = 100*Eigen::VectorXd::Ones(resolution + 1);

    std::vector<double> parameters = GPG->GetGPHyperparameters();
    parameters[2] = 1e-10; // disable long-range SE kernel
    parameters[6] = 1e-10; // disable short-range SE kernel
    parameters[7] = period_length; // set exact period length
    GPG->SetBoolComputePeriod(false); // use the exact period length
    GPG->SetGPHyperparameters(parameters);

    GPG->SetNumPointsForApproximation(1000); // need all data points for exact drift

    // feed data to the GPGuider
    for (int i = 0; i < timestamps.size(); ++i)
    {
        GPG->inject_data_point(timestamps[i], measurements[i], SNRs[i], controls[i]);
    }
    locations << 5000, 5000 + prediction_length;
    predictions = 0.25*locations; // only predict linear drift here
    // the first case is with an error smaller than min_move_
    EXPECT_NEAR(GPG->result(0.0, 2.0, prediction_length, max_time), predictions[1]-predictions[0], 2e-1);
}

TEST_F(GPGTest, data_preparation_test)
{   // no matter whether the gear function shows up in the controls or in the measurements,
    // the predictions should be identical

    // first: prepare a nice GP with a sine wave
    double period_length = 300;
    double max_time = 3*period_length;
    int resolution = 200;
    double prediction_length = 3.0;
    Eigen::VectorXd timestamps = Eigen::VectorXd::LinSpaced(resolution + 1, 0, max_time);
    Eigen::VectorXd measurements(timestamps.size());
    Eigen::VectorXd sine_data = 50*(timestamps.array()*2*M_PI/period_length).sin();
    Eigen::VectorXd controls(timestamps.size());
    Eigen::VectorXd SNRs = 100*Eigen::VectorXd::Ones(resolution + 1);

    // first option: the error was "compensated" and therefore only shows up in the controls
    controls << sine_data.tail(sine_data.size()-1) - sine_data.head(sine_data.size()-1), 0;
    measurements = 0*timestamps;

    // feed data to the GPGuider
    for (int i = 0; i < timestamps.size(); ++i)
    {
        GPG->inject_data_point(timestamps[i], measurements[i], SNRs[i], controls[i]);
    }
    double controlled_result = GPG->result(0.0, 2.0, prediction_length, max_time);
    GPG->reset();

    // second option: the error is not compensated and therefore visible in the measurement
    controls = 0*controls;
    measurements = sine_data;

    // feed data to the GPGuider
    for (int i = 0; i < timestamps.size(); ++i)
    {
        GPG->inject_data_point(timestamps[i], measurements[i], SNRs[i], controls[i]);
    }
    double measured_result = GPG->result(0.0, 2.0, prediction_length, max_time);

    EXPECT_NEAR(measured_result, controlled_result, 1e-1);
}

TEST_F(GPGTest, real_data_test)
{
    double time = 0.0;
    double measurement = 0.0;
    double SNR = 0.0;
    double control = 0.0;

    std::ifstream file("dataset01.csv");

    int i = 0;
    CSVRow row;
    while(file >> row)
    {
        // ignore special lines: "INFO", "Frame", "DROP"
        if (row[0][0] == 'I' || row[0][0] == 'F' || row[2][1] == 'D')
        {
            continue;
        }
        else
        {
            ++i;
        }
        time = std::stod(row[1]);
        measurement = std::stod(row[5]);
        control = std::stod(row[7]);
        SNR = std::stod(row[16]);

        GPG->inject_data_point(time, measurement, SNR, control);
    }

    GPG->result(0.0, 25.0, 3.0, time);

    EXPECT_NEAR(GPG->GetGPHyperparameters()[7],483.0,5);
}

TEST_F(GPGTest, parameter_filter_test)
{
    double period_length = 0.0;

    std::ifstream infile("dataset02.csv");

    Eigen::VectorXd filtered_period_lengths(1000); // must be longer than the dataset

    int i = 0;
    CSVRow row;
    while(infile >> row)
    {
        if (row[0][0] == 'p') // ignore the first line
        {
            continue;
        }
        else
        {
            ++i;
        }
        period_length = std::stod(row[0]);

        GPG->UpdatePeriodLength(period_length);
        filtered_period_lengths(i - 1) = GPG->GetGPHyperparameters()[7];
    }
    filtered_period_lengths.conservativeResize(i);
//     std::cout << filtered_period_lengths << std::endl;

    Eigen::VectorXd period_lengths_tail = filtered_period_lengths.tail(25);
    Eigen::VectorXd deviation = period_lengths_tail - 482*Eigen::VectorXd::Ones(25);

    EXPECT_LT(math_tools::stdandard_deviation(deviation), 1);
}

TEST_F(GPGTest, period_interpolation_test)
{
    // first: prepare a nice GP with a sine wave
    double period_length = 317;
    double max_time = 2345;
    int resolution = 527;
    Eigen::VectorXd timestamps = Eigen::VectorXd::LinSpaced(resolution + 1, 0, max_time);
    Eigen::VectorXd measurements = 50*(timestamps.array()*2*M_PI/period_length).sin();
    Eigen::VectorXd controls = 0*measurements;
    Eigen::VectorXd SNRs = 100*Eigen::VectorXd::Ones(resolution + 1);

    // feed data to the GPGuider
    for (int i = 0; i < timestamps.size(); ++i)
    {
        GPG->inject_data_point(timestamps[i], measurements[i], SNRs[i], controls[i]);
    }
    GPG->result(0.15, 2.0, 3.0);

    EXPECT_NEAR(GPG->GetGPHyperparameters()[7], period_length, 1e0);
}

TEST_F(GPGTest, data_regularization_test)
{
    // first: prepare a nice GP with a sine wave
    double period_length = 300;
    double max_time = 20000;
    int resolution = 8192;

    // second: mess up the grid of time stamps
    Eigen::VectorXd timestamps(resolution);
    timestamps << Eigen::VectorXd::LinSpaced(resolution/2, 0, max_time/6),
        Eigen::VectorXd::LinSpaced(resolution/2, max_time/6 + 0.5, max_time);
    timestamps += 0.5*math_tools::generate_normal_random_matrix(resolution, 1);

    Eigen::VectorXd measurements = 50*(timestamps.array()*2*M_PI/period_length).sin();
    Eigen::VectorXd controls = 0*measurements;
    Eigen::VectorXd SNRs = 100*Eigen::VectorXd::Ones(resolution + 1);

    // feed data to the GPGuider
    for (int i = 0; i < timestamps.size(); ++i)
    {
        GPG->inject_data_point(timestamps[i], measurements[i], SNRs[i], controls[i]);
    }
    GPG->result(0.15, 2.0, 3.0);

    EXPECT_NEAR(GPG->GetGPHyperparameters()[7], period_length, 1);
}

/**
 * This "test" is used to log the identified period length to file. This functionality
 * can be useful for debugging and for assessing the value of the period interpolation,
 * data regulatization and Kalman filtering techniques.
 */
TEST_F(GPGTest, DISABLED_log_period_length)
{
    double time = 0.0;
    double measurement = 0.0;
    double SNR = 0.0;
    double control = 0.0;

    std::ifstream file("dataset01.csv");

    std::ofstream outfile;
    outfile.open("period_lengths_reg_int_kf.csv", std::ios_base::out);
    outfile << "period_length\n";

    int i = 0;
    CSVRow row;
    while(file >> row)
    {
        // ignore special lines: "INFO", "Frame", "DROP"
        if (row[0][0] == 'I' || row[0][0] == 'F' || row[2][1] == 'D')
        {
            continue;
        }
        else
        {
            ++i;
        }
        time = std::stod(row[1]);
        measurement = std::stod(row[5]);
        control = std::stod(row[7]);
        SNR = std::stod(row[16]);

        GPG->inject_data_point(time, measurement, SNR, control);

        GPG->UpdateGP();
        outfile << std::setw(8) << GPG->GetGPHyperparameters()[7] << "\n";
    }
    outfile.close();
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
