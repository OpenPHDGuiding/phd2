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

#ifndef GUIDE_GAUSSIAN_PROCESS
#define GUIDE_GAUSSIAN_PROCESS

#include "guide_algorithm.h"
#include "gaussian_process_guider.h"

class wxStopWatch;

/**
 * This class provides a guiding algorithm for the right ascension axis that
 * learns and predicts the periodic gear error with a Gaussian process. This
 * prediction helps reducing periodic error components in the residual tracking
 * error. Further it is able to perform tracking without measurement to increase
 * robustness of the overall guiding system.
 */
class GuideAlgorithmGaussianProcess : public GuideAlgorithm
{
private:
    /**
     * Holds all data that is needed for the GP guiding.
     */
    struct gp_guide_parameters;

    /**
     * Provides the GUI configuration functionality.
     */
    class GuideAlgorithmGaussianProcessDialogPane;

    /**
     * Pointer to the class that does the actual work.
     */
    GaussianProcessGuider* GPG;

    /**
     * The expert mode shows more parameters in the configuration window.
     */
    bool expert_mode_;

    /**
     * Dark tracking mode is for debugging: only deduceResult is called if enabled.
     */
    bool dark_tracking_mode_;

protected:
    double GetControlGain() const;
    bool SetControlGain(double control_gain);

    double GetMinMove() const;
    bool SetMinMove(double min_move);

    double GetPeriodLengthsInference() const;
    bool SetPeriodLengthsInference(double);

    double GetPeriodLengthsPeriodEstimation() const;
    bool SetPeriodLengthsPeriodEstimation(double num_periods);

    int GetNumPointsForApproximation() const;
    bool SetNumPointsForApproximation(int);

    bool GetBoolComputePeriod() const;
    bool SetBoolComputePeriod(bool);

    std::vector<double> GetGPHyperparameters() const;
    bool SetGPHyperparameters(std::vector< double > hyperparameters);

    double GetPredictionGain() const;
    bool SetPredictionGain(double);

    bool GetDarkTracking();
    bool SetDarkTracking(bool value);

    bool GetExpertMode();
    bool SetExpertMode(bool value);

public:
    GuideAlgorithmGaussianProcess(Mount *pMount, GuideAxis axis);
    virtual ~GuideAlgorithmGaussianProcess(void);
    virtual GUIDE_ALGORITHM Algorithm(void);

    virtual ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);

    /**
     * Calculates the control value based on the current input. 1. The input is
     * stored, 2. the GP is updated with the new data point, 3. the prediction
     * is calculated to compensate the gear error and 4. the controller is
     * calculated, consisting of feedback and prediction parts.
     */
    virtual double result(double input);

    /**
     * This method provides predictive control if no measurement could be made.
     * A zero measurement is stored with high uncertainty, and then the GP
     * prediction is used for control.
     */
    virtual double deduceResult(void);

    /**
     * This method tells the guider that guiding was stopped, e.g. for
     * slweing. This method resets the internal state of the guider.
     */
    virtual void GuidingStopped(void);

    /**
     * This method tells the guider that guiding was paused, e.g. for
     * refocusing. This method keeps the internal state of the guider.
     */
    virtual void GuidingPaused(void);

    /**
     * This method tells the guider that guiding was resumed, e.g. after
     * refocusing. This method fills the measurements of the guider with
     * predictions to keep the FFT and the GP in a working state.
     */
    virtual void GuidingResumed(void);

    /**
     * This method tells the guider that a dither command was issued. The guider
     * will stop collecting measurements and uses predictions instead, to keep
     * the FFT and the GP working.
     */
    virtual void GuidingDithered(double amt);

    /**
     * This method tells the guider that dithering is finished. The guider
     * will resume normal operation.
     */
    virtual void GuidingDitherSettleDone(bool success);

    /**
     * Clears the data from the circular buffer and clears the GP data.
     */
    virtual void reset();

    virtual wxString GetSettingsSummary();
    virtual wxString GetGuideAlgorithmClassName(void) const { return "Predictive PEC"; }

};

#endif  // GUIDE_GAUSSIAN_PROCESS
