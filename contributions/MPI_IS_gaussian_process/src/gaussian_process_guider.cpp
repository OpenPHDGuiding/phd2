/**
 * PHD2 Guiding
 *
 * @file
 * @date      2014-2017
 * @copyright Max Planck Society
 *
 * @author    Edgar D. Klenske <edgar.klenske@tuebingen.mpg.de>
 * @author    Stephan Wenninger <stephan.wenninger@tuebingen.mpg.de>
 * @author    Raffi Enficiaud <raffi.enficiaud@tuebingen.mpg.de>
 *
 * @brief     Provides a Gaussian process based guiding algorithm
 */

/*
 *  This source code is distributed under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Bret McKee, Dad Dog Development, nor the names of its
 *     Craig Stark, Stark Labs nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include "gaussian_process_guider.h"

#include <cmath>
#include <ctime>

#define GP_DEBUG_FILE_ 0
#define CIRCULAR_BUFFER_SIZE 2048
#define FFT_SIZE 2048 // needs to be larger or equal than CIRCULAR_BUFFER_SIZE!

#if GP_DEBUG_FILE_
#include <iostream>
#include <iomanip>
#include <fstream>
#endif

GaussianProcessGuider::GaussianProcessGuider(guide_parameters parameters) :
    start_time_(std::chrono::system_clock::now()),
    last_time_(std::chrono::system_clock::now()),
    control_signal_(0),
    prediction_(0),
    last_prediction_end_(0),
    dither_steps_(0),
    dithering_active_(false),
    circular_buffer_data_(CIRCULAR_BUFFER_SIZE),
    parameters(parameters),
    covariance_function_(),
    output_covariance_function_(),
    gp_(covariance_function_)
{
    circular_buffer_data_.push_front(data_point()); // add first point
    circular_buffer_data_[0].control = 0; // set first control to zero
    gp_.enableExplicitTrend(); // enable the explicit basis function for the linear drift
    gp_.enableOutputProjection(output_covariance_function_); // for prediction

    std::vector<double> hyperparameters;
    hyperparameters.push_back(parameters.Noise_);
    hyperparameters.push_back(parameters.SE0KLengthScale_);
    hyperparameters.push_back(parameters.SE0KSignalVariance_);
    hyperparameters.push_back(parameters.PKLengthScale_);
    hyperparameters.push_back(parameters.PKSignalVariance_);
    hyperparameters.push_back(parameters.SE1KLengthScale_);
    hyperparameters.push_back(parameters.SE1KSignalVariance_);
    hyperparameters.push_back(parameters.PKPeriodLength_);
    SetGPHyperparameters(hyperparameters);
}

GaussianProcessGuider::~GaussianProcessGuider()
{
}

void GaussianProcessGuider::HandleTimestamps()
{
    auto current_time = std::chrono::system_clock::now();
    double delta_measurement_time = std::chrono::duration<double>(current_time - last_time_).count();
    last_time_ = current_time;
    get_last_point().timestamp = std::chrono::duration<double>(current_time - start_time_).count()
        - (delta_measurement_time / 2.0); // use the midpoint as time stamp
}

// adds a new measurement to the circular buffer that holds the data.
void GaussianProcessGuider::HandleMeasurements(double input)
{
    get_last_point().measurement = input;
}

void GaussianProcessGuider::HandleDarkGuiding()
{
    get_last_point().measurement = 0; // we didn't actually measure
    get_last_point().variance = 1e4; // add really high noise
}

void GaussianProcessGuider::HandleControls(double control_input)
{
    get_last_point().control = control_input;
}

void GaussianProcessGuider::HandleSNR(double SNR)
{
    SNR = std::max(SNR, 3.4); // limit the minimal SNR

    // this was determined by simulated experiments
    double standard_deviation = 2.1752 * 1 / (SNR - 3.3) + 0.5;

    get_last_point().variance = standard_deviation * standard_deviation;
}

void GaussianProcessGuider::UpdateGP()
{
    clock_t begin = std::clock(); // this is for timing the method in a simple way

    size_t N = get_number_of_measurements();

    // initialize the different vectors needed for the GP
    Eigen::VectorXd timestamps(N-1);
    Eigen::VectorXd measurements(N-1);
    Eigen::VectorXd variances(N-1);
    Eigen::VectorXd sum_controls(N-1);

    double sum_control = 0;

    // transfer the data from the circular buffer to the Eigen::Vectors
    for(size_t i = 0; i < N-1; i++)
    {
        sum_control += circular_buffer_data_[i].control; // sum over the control signals
        timestamps(i) = circular_buffer_data_[i].timestamp;
        measurements(i) = circular_buffer_data_[i].measurement;
        variances(i) = circular_buffer_data_[i].variance;
        sum_controls(i) = sum_control; // store current accumulated control signal
    }

    Eigen::VectorXd gear_error(N-1);
    Eigen::VectorXd linear_fit(N-1);

    // calculate the accumulated gear error
    gear_error = sum_controls + measurements; // for each time step, add the residual error

    clock_t end = std::clock();
    double time_init = double(end - begin) / CLOCKS_PER_SEC;
    begin = std::clock();

    // linear least squares regression for offset and drift to de-trend the data
    Eigen::MatrixXd feature_matrix(2, timestamps.rows());
    feature_matrix.row(0) = Eigen::MatrixXd::Ones(1, timestamps.rows()); // timestamps.pow(0)
    feature_matrix.row(1) = timestamps.array(); // timestamps.pow(1)

    // this is the inference for linear regression
    Eigen::VectorXd weights = (feature_matrix*feature_matrix.transpose()
    + 1e-3*Eigen::Matrix<double, 2, 2>::Identity()).ldlt().solve(feature_matrix*gear_error);

    // calculate the linear regression for all datapoints
    linear_fit = weights.transpose()*feature_matrix;

    // subtract polynomial fit from the data points
    Eigen::VectorXd gear_error_detrend = gear_error - linear_fit;

    end = std::clock();
    double time_detrend = double(end - begin) / CLOCKS_PER_SEC;
    begin = std::clock();

    double time_fft = 0; // need to initialize in case the FFT isn't calculated

    // calculate period length if we have enough points already
    size_t const min_points = static_cast<size_t>(parameters.min_points_for_period_computation_);
    if (parameters.compute_period_ && min_points > 0 && get_number_of_measurements() > min_points)
    {
        // find periodicity parameter with FFT

        // compute Hamming window to reduce spectral leakage
        Eigen::VectorXd windowed_gear_error = gear_error_detrend.array() * math_tools::hamming_window(gear_error_detrend.rows()).array();

        // compute the spectrum
        std::pair<Eigen::VectorXd, Eigen::VectorXd> result = math_tools::compute_spectrum(windowed_gear_error, FFT_SIZE);

        Eigen::ArrayXd amplitudes = result.first;
        Eigen::ArrayXd frequencies = result.second;

        double dt = (timestamps(timestamps.rows()-1) - timestamps(0))/(timestamps.rows()-1); // (t_end - t_begin) / num_t

        frequencies /= dt; // correct for the average time step width

        Eigen::ArrayXd periods = 1/frequencies.array();
        amplitudes = (periods > 1500.0).select(0,amplitudes); // set amplitudes to zero for too large periods

        assert(amplitudes.size() == frequencies.size());

        Eigen::VectorXd::Index maxIndex;
        amplitudes.maxCoeff(&maxIndex);
        double period_length = 1 / frequencies(maxIndex);

        std::vector<double> hypers = GetGPHyperparameters();
        hypers[7] = period_length;
        SetGPHyperparameters(hypers); // the setter function is needed to convert parameters

        end = std::clock();
        time_fft = double(end - begin) / CLOCKS_PER_SEC;

        #if GP_DEBUG_FILE_
        std::ofstream outfile;
        outfile.open("spectrum_data.csv", std::ios_base::out);
        if (outfile.is_open()) {
            outfile << "period, amplitude\n";
            for (int i = 0; i < amplitudes.size(); ++i) {
                outfile << std::setw(8) << periods[i] << "," << std::setw(8) << amplitudes[i] << "\n";
            }
        }
        else {
            std::cout << "unable to write to file" << std::endl;
        }
        outfile.close();
        #endif
    }

    begin = std::clock();

    // inference of the GP with the new points, maximum accuracy should be reached around current time
    gp_.inferSD(timestamps, gear_error, parameters.points_for_approximation_, variances,
                std::chrono::duration<double>(std::chrono::system_clock::now() - start_time_).count());

    end = std::clock();
    double time_gp = double(end - begin) / CLOCKS_PER_SEC;
//     Debug.AddLine(wxString::Format("timings: init: %f, detrend: %f, fft: %f, gp: %f", time_init, time_detrend, time_fft, time_gp));
}

double GaussianProcessGuider::PredictGearError(double prediction_location)
{
    // in the first step of each sequence, use the current time stamp as last prediction end
    if (last_prediction_end_ < 0.0)
    {
        last_prediction_end_ = std::chrono::duration<double>(std::chrono::system_clock::now() - start_time_).count();
    }

    // prediction from the last endpoint to the prediction point
    Eigen::VectorXd next_location(2);
    next_location << last_prediction_end_, prediction_location;
    Eigen::VectorXd prediction = gp_.predictProjected(next_location).first;

    double p1 = prediction(1);
    double p0 = prediction(0);
    assert(std::abs(p1 - p0) < 100); // large differences don't make sense
    assert(!math_tools::isNaN(p1 - p0));

    last_prediction_end_ = next_location(1); // store current endpoint

    // we are interested in the error introduced by the gear over the next time step
    return (p1 - p0);
}

double GaussianProcessGuider::result(double input, double SNR, double time_step, double prediction_point /*= -1*/)
{
    /*
     * Dithering behaves differently from pausing. During dithering, the mount
     * is moved and thus we can assume that we applied the perfect control, but
     * we cannot trust the measurement. Once dithering has settled, we can trust
     * the measurement again and we can pretend nothing has happend.
     */
    if (dithering_active_ == true)
    {
        dither_steps_--;
        if (dither_steps_ <= 0)
        {
            dithering_active_ = false;
        }
        deduceResult(time_step); // just pretend we would do dark guiding...
        return parameters.control_gain_*input; // ...but apply proportional control
    }

    // the starting time is set at the first call of result after startup or reset
    if (get_number_of_measurements() == 1)
    {
        start_time_ = std::chrono::system_clock::now();
        last_time_ = start_time_; // this is OK, since last_time_ only provides a minor correction
    }

    // collect data point content, except for the control signal
    HandleMeasurements(input);
    HandleTimestamps();
    HandleSNR(SNR);

    control_signal_ = parameters.control_gain_*input; // start with proportional control
    if (std::abs(control_signal_) < parameters.min_move_)
    {
        control_signal_ = 0; // don't make small moves
    }
    assert(std::abs(control_signal_) == 0 || std::abs(control_signal_) >= parameters.min_move_);

    // check if we are allowed to use the GP
    size_t const min_points = static_cast<size_t>(parameters.min_points_for_inference_);
    if (min_points > 0 && get_number_of_measurements() > min_points)
    {
        UpdateGP(); // update the GP based on the new measurements
        if (prediction_point < 0.0)
        {
            prediction_point = std::chrono::duration<double>(std::chrono::system_clock::now() - start_time_).count();
        }
        prediction_point += time_step;
        prediction_ = PredictGearError(prediction_point);
        control_signal_ += parameters.prediction_gain_*prediction_; // add the prediction
    }

    add_one_point(); // add new point here, since the control is for the next point in time
    HandleControls(control_signal_); // already store control signal

    // write the GP output to a file for easy analyzation
    #if GP_DEBUG_FILE_
    int N = get_number_of_measurements();

    // initialize the different vectors needed for the GP
    Eigen::VectorXd timestamps(N - 1);
    Eigen::VectorXd measurements(N - 1);
    Eigen::VectorXd variances(N - 1);
    Eigen::VectorXd controls(N - 1);
    Eigen::VectorXd sum_controls(N - 1);
    Eigen::VectorXd gear_error(N - 1);
    Eigen::VectorXd linear_fit(N - 1);

    // transfer the data from the circular buffer to the Eigen::Vectors
    for(size_t i = 0; i < N-1; i++)
    {
        timestamps(i) = circular_buffer_data_[i].timestamp;
        measurements(i) = circular_buffer_data_[i].measurement;
        variances(i) = circular_buffer_data_[i].variance;
        controls(i) = circular_buffer_data_[i].control;
        sum_controls(i) = circular_buffer_data_[i].control;
        if(i > 0)
        {
            sum_controls(i) += sum_controls(i-1); // sum over the control signals
        }
    }
    gear_error = sum_controls + measurements; // for each time step, add the residual error

    // inference of the GP with these new points
    gp_.inferSD(timestamps, gear_error, parameters.points_for_approximation_, variances,
                std::chrono::duration<double>(std::chrono::system_clock::now() - start_time_).count());

    int M = 512; // number of prediction points
    Eigen::VectorXd locations = Eigen::VectorXd::LinSpaced(M, 0, get_second_last_point().timestamp + 1500);

    std::pair<Eigen::VectorXd, Eigen::MatrixXd> predictions = gp_.predictProjected(locations);

    Eigen::VectorXd means = predictions.first;
    Eigen::VectorXd stds = predictions.second.diagonal().array().sqrt();

    std::ofstream outfile;
    outfile.open("measurement_data.csv", std::ios_base::out);
    if(outfile.is_open()) {
        outfile << "location, output\n";
        for( int i = 0; i < timestamps.size(); ++i) {
            outfile << std::setw(8) << timestamps[i] << "," << std::setw(8) << gear_error[i] << "\n";
        }
    } else {
        std::cout << "unable to write to file" << std::endl;
    }
    outfile.close();

    outfile.open("gp_data.csv", std::ios_base::out);
    if(outfile.is_open()) {
        outfile << "location, mean, std\n";
        for( int i = 0; i < locations.size(); ++i) {
            outfile << std::setw(8) << locations[i] << "," << std::setw(8) << means[i] << "," << std::setw(8) << stds[i] << "\n";
        }
    } else {
        std::cout << "unable to write to file" << std::endl;
    }
    outfile.close();
    #endif
//     Debug.AddLine(wxString::Format("GP Guider generated %f from input %f.", control_signal_, input));

    assert(std::abs(control_signal_) < 100); // such large control signals don't make sense
    assert(!math_tools::isNaN(control_signal_));
    return control_signal_;
}

double GaussianProcessGuider::deduceResult(double time_step, double prediction_point /*= -1.0*/)
{
    HandleDarkGuiding();
    HandleTimestamps();

    control_signal_ = 0; // no measurement!
    // check if we are allowed to use the GP
    size_t const min_points = static_cast<size_t>(parameters.min_points_for_inference_);
    if (min_points > 0 && get_number_of_measurements() > min_points)
    {
        UpdateGP(); // update the GP to update the SD approximation
        if (prediction_point < 0.0)
        {
            prediction_point = std::chrono::duration<double>(std::chrono::system_clock::now() - start_time_).count();
        }
        prediction_point += time_step;
        prediction_ = PredictGearError(prediction_point);
        control_signal_ += prediction_; // control based on prediction
    }

    add_one_point(); // add new point here, since the control is for the next point in time
    HandleControls(control_signal_); // already store control signal

    // write the GP output to a file for easy analyzation
    #if GP_DEBUG_FILE_
    int N = get_number_of_measurements();

    // initialize the different vectors needed for the GP
    Eigen::VectorXd timestamps(N - 1);
    Eigen::VectorXd measurements(N - 1);
    Eigen::VectorXd variances(N - 1);
    Eigen::VectorXd controls(N - 1);
    Eigen::VectorXd sum_controls(N - 1);
    Eigen::VectorXd gear_error(N - 1);
    Eigen::VectorXd linear_fit(N - 1);

    // transfer the data from the circular buffer to the Eigen::Vectors
    for (size_t i = 0; i < N - 1; i++)
    {
        timestamps(i) = circular_buffer_data_[i].timestamp;
        measurements(i) = circular_buffer_data_[i].measurement;
        variances(i) = circular_buffer_data_[i].variance;
        controls(i) = circular_buffer_data_[i].control;
        sum_controls(i) = circular_buffer_data_[i].control;
        if (i > 0)
        {
            sum_controls(i) += sum_controls(i - 1); // sum over the control signals
        }
    }
    gear_error = sum_controls + measurements; // for each time step, add the residual error

    // inference of the GP with these new points
    gp_.inferSD(timestamps, gear_error, parameters.points_for_approximation_, variances,
                std::chrono::duration<double>(std::chrono::system_clock::now() - start_time_).count());

    int M = 512; // number of prediction points
    Eigen::VectorXd locations = Eigen::VectorXd::LinSpaced(M, 0, get_second_last_point().timestamp + 1500);

    std::pair<Eigen::VectorXd, Eigen::MatrixXd> predictions = gp_.predictProjected(locations);

    Eigen::VectorXd means = predictions.first;
    Eigen::VectorXd stds = predictions.second.diagonal().array().sqrt();

    std::ofstream outfile;
    outfile.open("measurement_data.csv", std::ios_base::out);
    if (outfile.is_open()) {
        outfile << "location, output\n";
        for (int i = 0; i < timestamps.size(); ++i) {
            outfile << std::setw(8) << timestamps[i] << "," << std::setw(8) << gear_error[i] << "\n";
        }
    }
    else {
        std::cout << "unable to write to file" << std::endl;
    }
    outfile.close();

    outfile.open("gp_data.csv", std::ios_base::out);
    if (outfile.is_open()) {
        outfile << "location, mean, std\n";
        for (int i = 0; i < locations.size(); ++i) {
            outfile << std::setw(8) << locations[i] << "," << std::setw(8) << means[i] << "," << std::setw(8) << stds[i] << "\n";
        }
    }
    else {
        std::cout << "unable to write to file" << std::endl;
    }
    outfile.close();
    #endif

    assert(!math_tools::isNaN(control_signal_));
    return control_signal_;
}

void GaussianProcessGuider::reset()
{
    circular_buffer_data_.clear();
    circular_buffer_data_.push_front(data_point()); // add first point
    circular_buffer_data_[0].control = 0; // set first control to zero
    last_prediction_end_ = -1.0; // the negative value signals we didn't predict yet
    start_time_ = std::chrono::system_clock::now();
    last_time_ = std::chrono::system_clock::now();
    gp_.clearData();
}

void GaussianProcessGuider::GuidingStopped(void)
{
    reset(); // reset is only done on a complete stop
}

void GaussianProcessGuider::GuidingDithered(double amt)
{
    /*
     * We don't compensate for the dither amout (yet), but we need to know
     * that we are currently dithering.
     */
    dithering_active_ = true;
    dither_steps_ = 10;
}

void GaussianProcessGuider::GuidingDitherSettleDone(bool success)
{
    /*
     * Once dithering has settled, we can start regular guiding again.
     */
    if (success)
    {
        dithering_active_ = false;
        dither_steps_ = 0;
    }
}

double GaussianProcessGuider::GetControlGain(void) const
{
    return parameters.control_gain_;
}

bool GaussianProcessGuider::SetControlGain(double control_gain)
{
    parameters.control_gain_ = control_gain;
    return false;
}

bool GaussianProcessGuider::GetBoolComputePeriod() const {
    return parameters.compute_period_;
}

bool GaussianProcessGuider::SetBoolComputePeriod(bool active) {
    parameters.compute_period_ = active;
    return false;
}

std::vector<double> GaussianProcessGuider::GetGPHyperparameters() const
{
    // since the GP class works in log space, we have to exp() the parameters first.
    Eigen::VectorXd hyperparameters = gp_.getHyperParameters().array().exp();

    // converts the length-scale of the periodic covariance from standard notation to natural units
    hyperparameters(3) = std::asin(hyperparameters(3)/4)*hyperparameters(7)/M_PI;

    // we need to map the Eigen::vector into a std::vector.
    return std::vector<double>(hyperparameters.data(), // the first element is at the array address
                               hyperparameters.data() + 8); // 8 parameters, therefore the last is at position 7
}

bool GaussianProcessGuider::SetGPHyperparameters(std::vector<double> const &hyperparameters) {
    Eigen::VectorXd hyperparameters_eig = Eigen::VectorXd::Map(&hyperparameters[0], hyperparameters.size());

    // converts the length-scale of the periodic covariance from natural units to standard notation
    hyperparameters_eig(3) = 4*std::sin(hyperparameters_eig(3)*M_PI/hyperparameters_eig(7));

    // the GP works in log space, therefore we need to convert
    gp_.setHyperParameters(hyperparameters_eig.array().log());
    return false;
}

double GaussianProcessGuider::GetMinMove() const {
    return parameters.min_move_;
}

bool GaussianProcessGuider::SetMinMove(double min_move) {
    parameters.min_move_ = min_move;
    return false;
}

int GaussianProcessGuider::GetNumPointsForApproximation() const {
    return parameters.points_for_approximation_;
}

bool GaussianProcessGuider::SetNumPointsForApproximation(int num_points) {
    parameters.points_for_approximation_ = num_points;
    return false;
}

int GaussianProcessGuider::GetNumPointsInference() const {
    return parameters.min_points_for_inference_;
}

bool GaussianProcessGuider::SetNumPointsInference(int num_points_inference) {
    parameters.min_points_for_inference_ = num_points_inference;
    return false;
}

int GaussianProcessGuider::GetNumPointsPeriodComputation() const {
    return parameters.min_points_for_period_computation_;
}

bool GaussianProcessGuider::SetNumPointsPeriodComputation(int num_points) {
    parameters.min_points_for_period_computation_ = num_points;
    return false;
}

double GaussianProcessGuider::GetPredictionGain() const {
    return parameters.prediction_gain_;
}

bool GaussianProcessGuider::SetPredictionGain(double prediction_gain) {
    parameters.prediction_gain_ = prediction_gain;
    return false;
}

void GaussianProcessGuider::inject_data_point(double timestamp, double input, double SNR, double control) {
    // collect data point content, except for the control signal
    HandleMeasurements(input);
    last_prediction_end_ = timestamp;
    get_last_point().timestamp = timestamp; // overrides the usual HandleTimestamps();
    HandleSNR(SNR);

    start_time_ = std::chrono::system_clock::now() - std::chrono::seconds((int) timestamp);

    add_one_point(); // add new point here, since the control is for the next point in time
    HandleControls(control); // already store control signal
}





