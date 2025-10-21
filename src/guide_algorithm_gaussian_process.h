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
#include "mount.h" // for PierSide

#include <chrono>

class GaussianProcessGuider;

/**
 * This class provides a guiding algorithm for the right ascension axis that
 * learns and predicts the periodic gear error with a Gaussian process. This
 * prediction helps reducing periodic error components in the residual tracking
 * error. Further it is able to perform tracking without measurement to increase
 * robustness of the overall guiding system.
 */

class GuideAlgorithmGaussianProcess : public GuideAlgorithm
{
protected:
    class GPExpertDialog : public wxDialog
    {
        wxSpinCtrlDouble *m_pPeriodLengthsInference;
        wxSpinCtrlDouble *m_pPeriodLengthsPeriodEstimation;
        wxSpinCtrl *m_pNumPointsApproximation;
        wxSpinCtrlDouble *m_pSE0KLengthScale;
        wxSpinCtrlDouble *m_pSE0KSignalVariance;
        wxSpinCtrlDouble *m_pPKLengthScale;
        wxSpinCtrlDouble *m_pPKSignalVariance;
        wxSpinCtrlDouble *m_pSE1KLengthScale;
        wxSpinCtrlDouble *m_pSE1KSignalVariance;
        void AddTableEntry(wxFlexGridSizer *Grid, const wxString& Label, wxWindow *Ctrl, const wxString& ToolTip);

    public:
        GPExpertDialog(wxWindow *Parent);
        void LoadExpertValues(GuideAlgorithmGaussianProcess *m_pGuideAlgorithm, const std::vector<double>& hyperParams);
        void UnloadExpertValues(GuideAlgorithmGaussianProcess *m_pGuideAlgorithm, std::vector<double>& hyperParams);
    };

protected:
    class GuideAlgorithmGPGraphControlPane : public GraphControlPane
    {
    public:
        GuideAlgorithmGPGraphControlPane(wxWindow *pParent, GuideAlgorithmGaussianProcess *pAlgorithm, const wxString& label);
        ~GuideAlgorithmGPGraphControlPane();

    private:
        GuideAlgorithmGaussianProcess *m_pGuideAlgorithm;
        wxSpinCtrl *m_pWeight;
        wxSpinCtrl *m_pAggressiveness;
        wxSpinCtrlDouble *m_pMinMove;

        void OnWeightSpinCtrl(wxSpinEvent& evt);
        void OnAggressivenessSpinCtrl(wxSpinEvent& evt);
        void OnMinMoveSpinCtrlDouble(wxSpinDoubleEvent& evt);
    };

    GPExpertDialog *m_expertDialog; // Exactly one per GP instance independent from ConfigDialogPane lifetimes

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
    GaussianProcessGuider *GPG;

    /**
     * Dark tracking mode is for debugging: only deduceResult is called if enabled.
     */
    bool dark_tracking_mode_;
    bool block_updates_; // Don't update GP if guiding is disabled
    double guiding_ra_; // allow resuming guiding after guiding stopped if there is no change in RA
    PierSide guiding_pier_side_;
    std::chrono::steady_clock::time_point guiding_stopped_time_; // time guiding stopped

protected:
    double GetControlGain() const;
    bool SetControlGain(double control_gain);

    double GetMinMove() const override;
    bool SetMinMove(double min_move) override;

    double GetPeriodLengthsInference() const;
    bool SetPeriodLengthsInference(double);

    double GetPeriodLengthsPeriodEstimation() const;
    bool SetPeriodLengthsPeriodEstimation(double num_periods);

    int GetNumPointsForApproximation() const;
    bool SetNumPointsForApproximation(int);

    bool GetBoolComputePeriod() const;
    bool SetBoolComputePeriod(bool);

    std::vector<double> GetGPHyperparameters() const;
    bool SetGPHyperparameters(const std::vector<double>& hyperparameters);

    double GetPredictionGain() const;
    bool SetPredictionGain(double);

    bool GetDarkTracking() const;
    bool SetDarkTracking(bool value);

public:
    GuideAlgorithmGaussianProcess(Mount *pMount, GuideAxis axis);
    ~GuideAlgorithmGaussianProcess();
    GUIDE_ALGORITHM Algorithm() const override;

    ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent) override;
    GraphControlPane *GetGraphControlPane(wxWindow *pParent, const wxString& label) override;

    /**
     * Calculates the control value based on the current input. 1. The input is
     * stored, 2. the GP is updated with the new data point, 3. the prediction
     * is calculated to compensate the gear error and 4. the controller is
     * calculated, consisting of feedback and prediction parts.
     */
    double result(double input) override;

    /**
     * This method provides predictive control if no measurement could be made.
     * A zero measurement is stored with high uncertainty, and then the GP
     * prediction is used for control.
     */
    double deduceResult() override;

    /**
     * This method tells the guider that guiding has started.
     */
    void GuidingStarted() override;

    /**
     * This method tells the guider that guiding was stopped.
     */
    void GuidingStopped() override;

    /**
     * This method tells the guider that guiding was paused, e.g. for
     * refocusing. This method keeps the internal state of the guider.
     */
    void GuidingPaused() override;

    /**
     * This method tells the guider that guiding was resumed, e.g. after
     * refocusing. This method fills the measurements of the guider with
     * predictions to keep the FFT and the GP in a working state.
     */
    void GuidingResumed() override;

    /**
     * This method tells the guider that a dither command was issued. The guider
     * will stop collecting measurements and uses predictions instead, to keep
     * the FFT and the GP working.
     */
    void GuidingDithered(double amt) override;

    /**
     * This method tells the guider that dithering is finished. The guider
     * will resume normal operation.
     */
    void GuidingDitherSettleDone(bool success) override;

    /**
     * This method tells the guider that a direct move has been
     * applied. The Fast Recovery after Dither option applies direct
     * moves.
     */
    void DirectMoveApplied(double amt) override;

    /**
     * Clears the data from the circular buffer and clears the GP data.
     */
    void reset() override;

    // Tells guider that corrections are/not being sent to mount.  If
    // not, updates should not be applied to the GP algo, specifically
    // to avoid corruption of the period length
    void GuidingEnabled() override;
    void GuidingDisabled() override;

    wxString GetSettingsSummary() const override;
    wxString GetGuideAlgorithmClassName() const override { return "Predictive PEC"; }
    void GetParamNames(wxArrayString& names) const override;
    bool GetParam(const wxString& name, double *val) const override;
    bool SetParam(const wxString& name, double val) override;
};

#endif // GUIDE_GAUSSIAN_PROCESS
