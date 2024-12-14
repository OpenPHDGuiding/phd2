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

#ifndef GAUSSIAN_PROCESS_GUIDER
#define GAUSSIAN_PROCESS_GUIDER

#include "circbuf.h"
#include "gaussian_process.h"
#include "covariance_functions.h"
#include "math_tools.h"

#include <chrono>

enum Hyperparameters
{
    SE0KLengthScale,
    SE0KSignalVariance,
    PKLengthScale,
    PKSignalVariance,
    SE1KLengthScale,
    SE1KSignalVariance,
    PKPeriodLength,
    NumParameters // this represents the number of elements in the enum
};

/**
 * This class provides a guiding algorithm for the right ascension axis that
 * learns and predicts the periodic gear error with a Gaussian process. This
 * prediction helps reducing periodic error components in the residual tracking
 * error. Further it is able to perform tracking without measurement to increase
 * robustness of the overall guiding system.
 */
class GaussianProcessGuider
{
public:

    struct data_point
    {
        double timestamp;
        double measurement; // current pointing error
        double variance; // current measurement variance
        double control; // control action
    };

    /**
     * Holds all data that is needed for the GP guiding.
     */
    struct guide_parameters
    {
        double control_gain_;
        double min_move_;
        double prediction_gain_;

        double min_periods_for_inference_;
        double min_periods_for_period_estimation_;

        int points_for_approximation_;

        bool compute_period_;

        double SE0KLengthScale_;
        double SE0KSignalVariance_;
        double PKLengthScale_;
        double PKSignalVariance_;
        double SE1KLengthScale_;
        double SE1KSignalVariance_;
        double PKPeriodLength_;

        guide_parameters() :
            control_gain_(0.0),
            min_move_(0.0),
            prediction_gain_(0.0),
            min_periods_for_inference_(0.0),
            min_periods_for_period_estimation_(0.0),
            points_for_approximation_(0),
            compute_period_(false),
            SE0KLengthScale_(0.0),
            SE0KSignalVariance_(0.0),
            PKLengthScale_(0.0),
            PKSignalVariance_(0.0),
            SE1KLengthScale_(0.0),
            SE1KSignalVariance_(0.0),
            PKPeriodLength_(0.0)
        {
        }

    };

private:

    std::chrono::system_clock::time_point start_time_; // reference time
    std::chrono::system_clock::time_point last_time_;

    double control_signal_;
    double prediction_;
    double last_prediction_end_;

    int dither_steps_;
    bool dithering_active_;

    //! the dither offset collects the correction in gear time from dithering
    double dither_offset_;

    circular_buffer<data_point> circular_buffer_data_;

    covariance_functions::PeriodicSquareExponential2 covariance_function_; // for inference
    covariance_functions::PeriodicSquareExponential output_covariance_function_; // for prediction
    GP gp_;

    /**
     * Learning rate for smooth parameter adaptation.
     */
    double learning_rate_;

    /**
     * Guiding parameters of this instance.
     */
    guide_parameters parameters;

    /**
     * Stores the current time and creates a timestamp for the GP.
     */
    void SetTimestamp();

    /**
     * Stores the measurement, SNR and resets last_prediction_end_.
     */
    void HandleGuiding(double input, double SNR);

    /**
     * Stores a zero as blind "measurement" with high variance.
     */
    void HandleDarkGuiding();

    /**
     * Stores the control value.
     */
    void HandleControls(double control_input);

    /**
     * Calculates the noise from the reported SNR value according to an
     * empirically justified equation and stores it.
     */
    double CalculateVariance(double SNR);

    /**
     * Estimates the main period length for a given dataset.
     */
    double EstimatePeriodLength(const Eigen::VectorXd& time, const Eigen::VectorXd& data);

    /**
     * Calculates the difference in gear error for the time between the last
     * prediction point and the current prediction point, which lies one
     * exposure length in the future.
     */
    double PredictGearError(double prediction_location);



public:
    double GetControlGain() const;
    bool SetControlGain(double control_gain);

    double GetMinMove() const;
    bool SetMinMove(double min_move);

    double GetPeriodLengthsInference() const;
    bool SetPeriodLengthsInference(double num_periods);

    double GetPeriodLengthsPeriodEstimation() const;
    bool SetPeriodLengthsPeriodEstimation(double num_periods);

    int GetNumPointsForApproximation() const;
    bool SetNumPointsForApproximation(int num_points);

    bool GetBoolComputePeriod() const;
    bool SetBoolComputePeriod(bool active);

    std::vector<double> GetGPHyperparameters() const;
    bool SetGPHyperparameters(const std::vector<double>& hyperparameters);

    double GetPredictionGain() const;
    bool SetPredictionGain(double);

    GaussianProcessGuider(guide_parameters parameters);
    ~GaussianProcessGuider();

    /**
     * Calculates the control value based on the current input. 1. The input is
     * stored, 2. the GP is updated with the new data point, 3. the prediction
     * is calculated to compensate the gear error and 4. the controller is
     * calculated, consisting of feedback and prediction parts.
     */
    double result(double input, double SNR, double time_step, double prediction_point = -1.0);

    /**
     * This method provides predictive control if no measurement could be made.
     * A zero measurement is stored with high uncertainty, and then the GP
     * prediction is used for control.
     */
    double deduceResult(double time_step, double prediction_point = -1.0);

    /**
     * This method tells the guider that a dither command was issued. The guider
     * will stop collecting measurements and uses predictions instead, to keep
     * the FFT and the GP working.
     */
    void GuidingDithered(double amt, double rate);

    /**
     * This method tells the guider that a direct move was
     * issued. This has the opposite effect of a dither on the dither
     * offset.
     */
    void DirectMoveApplied(double amt, double rate);

    /**
     * This method tells the guider that dithering is finished. The guider
     * will resume normal operation.
     */
    void GuidingDitherSettleDone(bool success);

    /**
     * Clears the data from the circular buffer and clears the GP data.
     */
    void reset();

    /**
     * Runs the inference machinery on the GP. Gets the measurement data from
     * the circular buffer and stores it in Eigen::Vectors. Detrends the data
     * with linear regression. Calculates the main frequency with an FFT.
     * Updates the GP accordingly with new data and parameter.
     */
    void UpdateGP(double prediction_point = std::numeric_limits<double>::quiet_NaN());

    /**
     * Does filtering and sets the period length of the GPGuider.
     */
    void UpdatePeriodLength(double period_length);

    data_point& get_last_point() const
    {
        return circular_buffer_data_[circular_buffer_data_.size() - 1];
    }

    data_point& get_second_last_point() const
    {
        return circular_buffer_data_[circular_buffer_data_.size() - 2];
    }

    size_t get_number_of_measurements() const
    {
        return circular_buffer_data_.size();
    }

    void add_one_point()
    {
        circular_buffer_data_.push_front(data_point());
    }

    /**
     * This method is needed for automated testing. It can inject data points.
     */
    void inject_data_point(double timestamp, double input, double SNR, double control);

    /**
     * Takes timestamps, measurements and SNRs and returns them regularized in a matrix.
     */
    Eigen::MatrixXd regularize_dataset(const Eigen::VectorXd& timestamps, const Eigen::VectorXd& gear_error, const Eigen::VectorXd& variances);

    /**
     * Saves the GP data to a csv file for external analysis. Expensive!
     */
    void save_gp_data() const;

    /**
     * Sets the learning rate. Useful for disabling it for testing.
     */
    void SetLearningRate(double learning_rate);
};

//
// GPDebug abstract interface to allow logging to the PHD2 Debug Log when
// GaussianProcessGuider is built in the context of the PHD2 application
//
// Add code like this to record debug info in the PHD2 debug log (with newline appended)
//
//   GPDebug->Log("input: %.2f SNR: %.1f time_step: %.1f", input, SNR, time_step);
//
// Outside of PHD2, like in the test framework, these calls will not produce any output
//
class GPDebug
{
public:
    static void SetGPDebug(GPDebug *logger);
    virtual ~GPDebug();
    virtual void Log(const char *format, ...) = 0;
    virtual void Write(const char *what) = 0;
};

extern class GPDebug *GPDebug;

#endif  // GAUSSIAN_PROCESS_GUIDER
