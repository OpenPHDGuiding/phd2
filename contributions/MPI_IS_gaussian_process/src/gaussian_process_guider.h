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

#define GP_DEBUG_FILE_ 1

#if GP_DEBUG_FILE_
#include <iostream>
#include <iomanip>
#include <fstream>
#endif

#include "circbuf.h"
#include "gaussian_process.h"
#include "covariance_functions.h"
#include "math_tools.h"

#define CIRCULAR_BUFFER_SIZE 2048
#define FFT_SIZE 2048 // needs to be larger or equal than CIRCULAR_BUFFER_SIZE!

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

        int min_points_for_inference_;
        int min_points_for_period_computation_;
        int points_for_approximation_;

        bool compute_period_;

        double Noise_;
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
        min_points_for_inference_(0),
        min_points_for_period_computation_(0),
        points_for_approximation_(0),
        compute_period_(false),
        Noise_(0.0),
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

    double time_start;
    double control_signal_;
    double last_timestamp_;
    double prediction_;
    double last_prediction_end_;

    int dither_steps_;

    bool dithering_active_;

    circular_buffer<data_point> circular_buffer_data_;

    covariance_functions::PeriodicSquareExponential2 covariance_function_; // for inference
    covariance_functions::PeriodicSquareExponential output_covariance_function_; // for prediction
    GP gp_;

    /**
     * Guiding parameters of this instance.
     */
    guide_parameters parameters;

    /**
     * Stores the current time and creates a timestamp for the GP.
     */
    void HandleTimestamps();

    /**
     * Stores the measurement to the last datapoint.
     */
    void HandleMeasurements(double input);

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
    void HandleSNR(double SNR);

    /**
     * Runs the inference machinery on the GP. Gets the measurement data from
     * the circular buffer and stores it in Eigen::Vectors. Detrends the data
     * with linear regression. Calculates the main frequency with an FFT.
     * Updates the GP accordingly with new data and parameter.
     */
    void UpdateGP();

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

    int GetNumPointsInference() const;
    bool SetNumPointsInference(int num_points_inference);

    int GetNumPointsPeriodComputation() const;
    bool SetNumPointsPeriodComputation(int num_points);

    int GetNumPointsForApproximation() const;
    bool SetNumPointsForApproximation(int num_points);

    bool GetBoolComputePeriod() const;
    bool SetBoolComputePeriod(bool active);

    std::vector< double > GetGPHyperparameters() const;
    bool SetGPHyperparameters(const std::vector< double >& hyperparameters);

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
    double result(double input, double SNR, double time_step);

    /**
     * This method provides predictive control if no measurement could be made.
     * A zero measurement is stored with high uncertainty, and then the GP
     * prediction is used for control.
     */
    double deduceResult(double time_step);

    /**
     * This method tells the guider that guiding was stopped, e.g. for
     * slweing. This method resets the internal state of the guider.
     */
    void GuidingStopped(void);

    /**
     * This method tells the guider that guiding was paused, e.g. for
     * refocusing. This method keeps the internal state of the guider.
     */
    void GuidingPaused(void);

    /**
     * This method tells the guider that guiding was resumed, e.g. after
     * refocusing. This method fills the measurements of the guider with
     * predictions to keep the FFT and the GP in a working state.
     */
    void GuidingResumed(void);

    /**
     * This method tells the guider that a dither command was issued. The guider
     * will stop collecting measurements and uses predictions instead, to keep
     * the FFT and the GP working.
     */
    void GuidingDithered(double amt);

    /**
     * This method tells the guider that dithering is finished. The guider
     * will resume normal operation.
     */
    void GuidingDitherSettleDone(bool success);

    /**
     * Clears the data from the circular buffer and clears the GP data.
     */
    void reset();

    data_point& get_last_point()
    {
        return circular_buffer_data_[circular_buffer_data_.size() - 1];
    }

    data_point& get_second_last_point()
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

    void clear()
    {
        circular_buffer_data_.clear();
        circular_buffer_data_.push_front(data_point()); // add first point
        circular_buffer_data_[0].control = 0; // set first control to zero
        last_prediction_end_ = 0.0;
        gp_.clearData();
    }

};




#endif  // GAUSSIAN_PROCESS_GUIDER
