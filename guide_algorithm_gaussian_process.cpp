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


#include "phd.h"

#include "guide_algorithm_gaussian_process.h"
#include <wx/stopwatch.h>
#include <ctime>

#include "math_tools.h"

#include "math_tools.h"
#include "gaussian_process.h"
#include "covariance_functions.h"

/** Default values for the parameters of this algorithm */

static const double DefaultControlGain                   = 0.6; // control gain
static const int    DefaultNumMinPointsForInference      = 100; // minimal number of points for doing the inference
static const double DefaultMinMove                       = 0.01;

static const double DefaultGaussianNoiseHyperparameter   = 1.0; // default Gaussian measurement noise

static const double DefaultLengthScaleSE0Ker             = 500.0; // length-scale of the long-range SE-kernel
static const double DefaultSignalVarianceSE0Ker          = 20.0; // signal variance of the long-range SE-kernel
static const double DefaultLengthScalePerKer             = 25.0; // length-scale of the periodic kernel
static const double DefaultPeriodLengthPerKer            = 500.0; // P_p, period-length of the periodic kernel
static const double DefaultSignalVariancePerKer          = 30.0; // signal variance of the periodic kernel
static const double DefaultLengthScaleSE1Ker             = 7.0; // length-scale of the short-range SE-kernel
static const double DefaultSignalVarianceSE1Ker          = 10.0; // signal variance of the short range SE-kernel

static const int    DefaultNumMinPointsForPeriodComputation = 240; // minimal number of points for doing the period identification
static const int    DefaultNumPointsForApproximation        = 100; // number of points used in the GP approximation
static const double DefaultPredictionGain                  = 0.8; // amount of GP prediction to blend in

static const bool   DefaultComputePeriod                 = true;

class GuideAlgorithmGaussianProcess::GuideAlgorithmGaussianProcessDialogPane : public wxEvtHandler, public ConfigDialogPane
{
    GuideAlgorithmGaussianProcess *m_pGuideAlgorithm;
    wxSpinCtrlDouble *m_pControlGain;
    wxSpinCtrlDouble *m_pMinMove;
    wxSpinCtrl       *m_pNumPointsInference;
    wxSpinCtrl       *m_pNumPointsPeriodComputation;
    wxSpinCtrl       *m_pNumPointsApproximation;

    wxSpinCtrlDouble *m_pSE0KLengthScale;
    wxSpinCtrlDouble *m_pSE0KSignalVariance;
    wxSpinCtrlDouble *m_pPKLengthScale;
    wxSpinCtrlDouble *m_pPKPeriodLength;
    wxSpinCtrlDouble *m_pPKSignalVariance;
    wxSpinCtrlDouble *m_pSE1KLengthScale;
    wxSpinCtrlDouble *m_pSE1KSignalVariance;
    wxSpinCtrlDouble *m_pPredictionGain;

    wxCheckBox       *m_checkboxComputePeriod;

    wxCheckBox       *m_checkboxDarkMode;

    wxCheckBox       *m_checkboxExpertMode;

    wxBoxSizer       *m_pExpertPage;

public:
    GuideAlgorithmGaussianProcessDialogPane(wxWindow *pParent, GuideAlgorithmGaussianProcess *pGuideAlgorithm)
      : ConfigDialogPane(_("Predictive PEC Guide Algorithm"),pParent)
    {
        m_pGuideAlgorithm = pGuideAlgorithm;

        int width;

        width = StringWidth(_T("0.00"));
        m_pControlGain = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, _T(" "), wxDefaultPosition,
            wxSize(width, -1), wxSP_ARROW_KEYS, 0.0, 1.0, DefaultControlGain, 0.05);
        m_pControlGain->SetDigits(2);

        DoAdd(_("Control Gain"), m_pControlGain,
            wxString::Format(_("The control gain defines the aggressiveness of the controller. "
            "It is the amount of pointing error that is fed back to the system. Default = %.2f"), DefaultControlGain));

        width = StringWidth(_T("0.00"));
        m_pPredictionGain = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, _T(" "), wxDefaultPosition,
            wxSize(width, -1), wxSP_ARROW_KEYS, 0.0, 1.0, DefaultPredictionGain, 0.05);
        m_pPredictionGain->SetDigits(2);
        DoAdd(_("Prediction Gain"), m_pPredictionGain,
            wxString::Format(_("The prediction gain defines how much of the prediction is used for control. "
            "Default = %.2f"), DefaultPredictionGain));

        width = StringWidth(_T("0.00"));
        m_pMinMove = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, _T(" "), wxDefaultPosition,
            wxSize(width, -1), wxSP_ARROW_KEYS, 0.0, 20.0, DefaultMinMove, 0.05);
        m_pMinMove->SetDigits(2);
        DoAdd(_("Minimum Move (pixels)"), m_pMinMove,
            wxString::Format(_("How many (fractional) pixels must the star move to trigger a guide pulse? "
            "If camera is binned, this is a fraction of the binned pixel size. Note that the movements from "
            "the prediction are not affected by this. Default = %.2f"), DefaultMinMove));

        width = StringWidth(_T("0000.0"));
        m_pPKPeriodLength = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, _T(" "), wxDefaultPosition,
            wxSize(width, -1), wxSP_ARROW_KEYS, 50, 2000, DefaultPeriodLengthPerKer, 1);
        m_pPKPeriodLength->SetDigits(2);
        m_checkboxComputePeriod = new wxCheckBox(pParent, wxID_ANY, _T("auto"));
        DoAdd(_("Period Length"), m_pPKPeriodLength,
            wxString::Format(_("The period length (in seconds) of the periodic error component that should be "
            "corrected. Default = %.2f"), DefaultPeriodLengthPerKer), m_checkboxComputePeriod);

        width = StringWidth(_T("0000"));
        m_pNumPointsApproximation = pFrame->MakeSpinCtrl(pParent, wxID_ANY, _T(" "), wxDefaultPosition,
            wxSize(width, -1), wxSP_ARROW_KEYS, 0, 2000, DefaultNumPointsForApproximation);
        DoAdd(_("Approximation Data Points"), m_pNumPointsApproximation,
            wxString::Format(_("Number of data points used in the approximation. Both prediction accuracy "
            "as well as runtime rise with the number of datapoints. Default = %d"), DefaultNumPointsForApproximation));

        // create the expert options page
        m_pExpertPage = new wxBoxSizer(wxVERTICAL);

        width = StringWidth(_T("0000"));
        m_pNumPointsInference = pFrame->MakeSpinCtrl(pParent, wxID_ANY, _T(" "), wxDefaultPosition,
            wxSize(width, -1), wxSP_ARROW_KEYS, 0, 1000, DefaultNumMinPointsForInference);
        m_pExpertPage->Add(MakeLabeledControl(_("Minimum Data Points (Prediction)"), m_pNumPointsInference,
            wxString::Format(_("Minimal number of measurements needed to use the prediction. "
            "If there are too little data points, the prediction might be poor. "
            "Default = %d"), DefaultNumMinPointsForInference)));

        width = StringWidth(_T("0000"));
        m_pNumPointsPeriodComputation = pFrame->MakeSpinCtrl(pParent, wxID_ANY, _T(" "), wxDefaultPosition,
            wxSize(width, -1), wxSP_ARROW_KEYS, 0, DefaultNumMinPointsForPeriodComputation, 10);
        m_pExpertPage->Add(MakeLabeledControl(_("Minimum Data Points (Period Estimation)"), m_pNumPointsPeriodComputation,
            wxString::Format(_("Minimal number of measurements for estimating the period length. "
            "If there are too little data points, the estimation might not work. Default = %d"),
            DefaultNumMinPointsForPeriodComputation)));

        width = StringWidth(_T("0000.0"));
        m_pSE0KLengthScale = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, _T(" "), wxDefaultPosition,
            wxSize(width, -1), wxSP_ARROW_KEYS, 100.0, 5000.0, DefaultLengthScaleSE0Ker, 10.0);
        m_pSE0KLengthScale->SetDigits(2);
        m_pExpertPage->Add(MakeLabeledControl(_("Length Scale (Long Range)"), m_pSE0KLengthScale,
            wxString::Format(_("The length scale (in seconds) of the large non-periodic structure. "
            "This is essentially a high-pass filter for the periodic error and the length scale "
            "defines the corner frequency. Default = %.2f"),
            DefaultLengthScaleSE0Ker)));

        width = StringWidth(_T("000.0"));
        m_pSE0KSignalVariance = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, _T(" "), wxDefaultPosition,
            wxSize(width, -1), wxSP_ARROW_KEYS, 0.0, 100.0, DefaultSignalVarianceSE0Ker, 0.1);
        m_pSE0KSignalVariance->SetDigits(2);
        m_pExpertPage->Add(MakeLabeledControl(_("Signal Variance (Long Range)"), m_pSE0KSignalVariance,
            wxString::Format(_("Signal variance (in pixels) of the long-term variations. Default = %.2f"), DefaultSignalVarianceSE0Ker)));

        width = StringWidth(_T("000.0"));
        m_pPKLengthScale = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, _T(" "), wxDefaultPosition,
            wxSize(width, -1), wxSP_ARROW_KEYS, 0.0, 10.0, DefaultLengthScalePerKer, 0.05);
        m_pPKLengthScale->SetDigits(2);
        m_pExpertPage->Add(MakeLabeledControl(_("Length Scale (Periodic)"), m_pPKLengthScale,
            wxString::Format(_("The length scale (in seconds) defines the \"wigglyness\" of the periodic structure. "
            "The smaller the length scale, the more structure can be learned. If chosen too "
            "small, some non-periodic structure might be picked up as well. Default = %.2f"), DefaultLengthScalePerKer)));

        width = StringWidth(_T("000.0"));
        m_pPKSignalVariance = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, _T(" "), wxDefaultPosition,
            wxSize(width, -1), wxSP_ARROW_KEYS, 0.0, 100.0, DefaultSignalVariancePerKer, 0.1);
        m_pPKSignalVariance->SetDigits(2);
        m_pExpertPage->Add(MakeLabeledControl(_("Signal Variance (Periodic)"), m_pPKSignalVariance,
            wxString::Format(_("The signal variance (in pixels) of the periodic error. "
            "Default = %.2f"), DefaultSignalVariancePerKer)));

        width = StringWidth(_T("000.0"));
        m_pSE1KLengthScale = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, _T(" "), wxDefaultPosition,
            wxSize(width, -1), wxSP_ARROW_KEYS, 0.0, 100.0, DefaultLengthScaleSE1Ker, 1.0);
        m_pSE1KLengthScale->SetDigits(2);
        m_pExpertPage->Add(MakeLabeledControl(_("Length Scale (Short Range)"), m_pSE1KLengthScale,
            wxString::Format(_("Length scale (in seconds) of the short range non-periodic parts "
            "of the gear error. This is essentially a low-pass filter for the periodic error and the length "
            "scale defines the corner frequency. Default = %.2f"), DefaultLengthScaleSE1Ker)));

        width = StringWidth(_T("000.0"));
        m_pSE1KSignalVariance = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, _T(" "), wxDefaultPosition,
            wxSize(width, -1), wxSP_ARROW_KEYS, 0.0, 100.0, DefaultSignalVarianceSE1Ker, 0.1);
        m_pSE1KSignalVariance->SetDigits(2);
        m_pExpertPage->Add(MakeLabeledControl(_("Signal Variance (Short Range)"), m_pSE1KSignalVariance,
            wxString::Format(_("Signal variance (in pixels) of the short-term variations. Default = %.2f"), DefaultSignalVarianceSE1Ker)));

        m_checkboxExpertMode = new wxCheckBox(pParent, wxID_ANY, _T(""));
        pParent->Connect(m_checkboxExpertMode->GetId(), wxEVT_CHECKBOX,
            wxCommandEventHandler(GuideAlgorithmGaussianProcess::GuideAlgorithmGaussianProcessDialogPane::EnableExpertMode),
            0, this);
        DoAdd(_("Show expert options"), m_checkboxExpertMode, _("Shows the expert options for tuning the predictions. "
            "Use at your own risk!"));

        m_checkboxDarkMode = new wxCheckBox(pParent, wxID_ANY, "");
        m_pExpertPage->Add(MakeLabeledControl(_("Dark Guiding Mode"), m_checkboxDarkMode,
            _("Disables the use of the measurements, useful only for testing.")));

        // add expert options to the main options
        DoAdd(m_pExpertPage);
    }

    virtual ~GuideAlgorithmGaussianProcessDialogPane(void)
    {
      // no need to destroy the widgets, this is done by the parent...
    }

    /* Fill the GUI with the parameters that are currently configured in the
     * guiding algorithm.
     */
    virtual void LoadValues(void)
    {
        m_pControlGain->SetValue(m_pGuideAlgorithm->GetControlGain());
        m_pPredictionGain->SetValue(m_pGuideAlgorithm->GetPredictionGain());
        m_pMinMove->SetValue(m_pGuideAlgorithm->GetMinMove());
        m_pNumPointsInference->SetValue(m_pGuideAlgorithm->GetNumPointsInference());
        m_pNumPointsPeriodComputation->SetValue(m_pGuideAlgorithm->GetNumPointsPeriodComputation());
        m_pNumPointsApproximation->SetValue(m_pGuideAlgorithm->GetNumPointsForApproximation());

        std::vector<double> hyperparameters = m_pGuideAlgorithm->GetGPHyperparameters();
        assert(hyperparameters.size() == 8);

        m_pSE0KLengthScale->SetValue(hyperparameters[1]);
        m_pSE0KSignalVariance->SetValue(hyperparameters[2]);
        m_pPKLengthScale->SetValue(hyperparameters[3]);
        m_pPKSignalVariance->SetValue(hyperparameters[4]);
        m_pSE1KLengthScale->SetValue(hyperparameters[5]);
        m_pSE1KSignalVariance->SetValue(hyperparameters[6]);
        m_pPKPeriodLength->SetValue(hyperparameters[7]);

        m_checkboxComputePeriod->SetValue(m_pGuideAlgorithm->GetBoolComputePeriod());

        m_pExpertPage->ShowItems(m_pGuideAlgorithm->GetExpertMode());
        m_pExpertPage->Layout();
        m_pParent->Layout();
    }

    // Set the parameters chosen in the GUI in the actual guiding algorithm
    virtual void UnloadValues(void)
    {
        m_pGuideAlgorithm->SetControlGain(m_pControlGain->GetValue());
        m_pGuideAlgorithm->SetPredictionGain(m_pPredictionGain->GetValue());
        m_pGuideAlgorithm->SetMinMove(m_pMinMove->GetValue());
        m_pGuideAlgorithm->SetNumPointsInference(m_pNumPointsInference->GetValue());
        m_pGuideAlgorithm->SetNumPointsPeriodComputation(m_pNumPointsPeriodComputation->GetValue());
        m_pGuideAlgorithm->SetNumPointsForApproximation(m_pNumPointsApproximation->GetValue());

        std::vector<double> hyperparameters(8);

        hyperparameters[1] = m_pSE0KLengthScale->GetValue();
        hyperparameters[2] = m_pSE0KSignalVariance->GetValue();
        hyperparameters[3] = m_pPKLengthScale->GetValue();
        hyperparameters[4] = m_pPKSignalVariance->GetValue();
        hyperparameters[5] = m_pSE1KLengthScale->GetValue();
        hyperparameters[6] = m_pSE1KSignalVariance->GetValue();
        hyperparameters[7] = m_pPKPeriodLength->GetValue();

        m_pGuideAlgorithm->SetGPHyperparameters(hyperparameters);
        m_pGuideAlgorithm->SetBoolComputePeriod(m_checkboxComputePeriod->GetValue());

        m_pGuideAlgorithm->SetExpertMode(m_checkboxExpertMode->GetValue());
    }

    virtual void EnableExpertMode(wxCommandEvent& evt)
    {
        m_pExpertPage->ShowItems(evt.IsChecked());
        m_pExpertPage->Layout();
        m_pParent->Layout();
    }
};


GuideAlgorithmGaussianProcess::GuideAlgorithmGaussianProcess(Mount *pMount, GuideAxis axis)
    : GuideAlgorithm(pMount, axis), gpg_(0)
{
    // create guide parameters, load default values at first
    GaussianProcessGuider::guide_parameters parameters;
    parameters.control_gain_ = DefaultControlGain;
    parameters.min_points_for_inference_ = DefaultNumMinPointsForInference;
    parameters.min_move_ = DefaultMinMove;
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

    // create instance of the worker
    gpg_ = new GaussianProcessGuider(parameters);

    wxString configPath = GetConfigPath();

    double control_gain = pConfig->Profile.GetDouble(configPath + "/gp_control_gain", DefaultControlGain);
    SetControlGain(control_gain);

    double min_move = pConfig->Profile.GetDouble(configPath + "/gp_min_move", DefaultMinMove);
    SetMinMove(min_move);

    int num_element_for_inference = pConfig->Profile.GetInt(configPath + "/gp_min_points_inference", DefaultNumMinPointsForInference);
    SetNumPointsInference(num_element_for_inference);

    int num_points_period_computation = pConfig->Profile.GetInt(configPath + "/gp_min_points_period_computation", DefaultNumMinPointsForPeriodComputation);
    SetNumPointsPeriodComputation(num_points_period_computation);

    int num_points_approximation = pConfig->Profile.GetInt(configPath + "/gp_points_for_approximation", DefaultNumPointsForApproximation);
    SetNumPointsForApproximation(num_points_approximation);

    double prediction_gain = pConfig->Profile.GetDouble(configPath + "/gp_prediction_gain", DefaultPredictionGain);
    SetPredictionGain(prediction_gain);

    std::vector<double> v_hyperparameters(8);
    v_hyperparameters[0] = pConfig->Profile.GetDouble(configPath + "/gp_gaussian_noise",         DefaultGaussianNoiseHyperparameter);
    v_hyperparameters[1] = pConfig->Profile.GetDouble(configPath + "/gp_length_scale_se0_kern",  DefaultLengthScaleSE0Ker);
    v_hyperparameters[2] = pConfig->Profile.GetDouble(configPath + "/gp_sigvar_se0_kern",        DefaultSignalVarianceSE0Ker);
    v_hyperparameters[3] = pConfig->Profile.GetDouble(configPath + "/gp_length_scale_per_kern",  DefaultLengthScalePerKer);
    v_hyperparameters[4] = pConfig->Profile.GetDouble(configPath + "/gp_sigvar_per_kern",        DefaultSignalVariancePerKer);
    v_hyperparameters[5] = pConfig->Profile.GetDouble(configPath + "/gp_length_scale_se1_kern",  DefaultLengthScaleSE1Ker);
    v_hyperparameters[6] = pConfig->Profile.GetDouble(configPath + "/gp_sigvar_se1_kern",        DefaultSignalVarianceSE1Ker);
    v_hyperparameters[7] = pConfig->Profile.GetDouble(configPath + "/gp_period_per_kern",        DefaultPeriodLengthPerKer);

    SetGPHyperparameters(v_hyperparameters);

    bool compute_period = pConfig->Profile.GetBoolean(configPath + "/gp_compute_period", DefaultComputePeriod);
    SetBoolComputePeriod(compute_period);

    dark_tracking_mode_ = false; // dark tracking mode ignores measurements
    SetExpertMode(false); // expert mode exposes the GP hyperparameters
    reset();
}

GuideAlgorithmGaussianProcess::~GuideAlgorithmGaussianProcess(void)
{
    delete gpg_;
}


ConfigDialogPane *GuideAlgorithmGaussianProcess::GetConfigDialogPane(wxWindow *pParent)
{
    return new GuideAlgorithmGaussianProcessDialogPane(pParent, this);
}

bool GuideAlgorithmGaussianProcess::SetControlGain(double control_gain)
{
    bool error = false;

    try
    {
        if (control_gain < 0)
        {
            throw ERROR_INFO("invalid controlGain");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        control_gain = DefaultControlGain;
    }

    gpg_->SetControlGain(control_gain);

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_control_gain", control_gain);

    return error;
}

bool GuideAlgorithmGaussianProcess::SetMinMove(double min_move)
{
    bool error = false;

    try
    {
        if (min_move < 0)
        {
            throw ERROR_INFO("invalid minimum move");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        min_move = DefaultMinMove;
    }

    gpg_->SetMinMove(min_move);

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_min_move", min_move);

    return error;
}

bool GuideAlgorithmGaussianProcess::SetNumPointsInference(int num_elements)
{
    bool error = false;

    try
    {
        if (num_elements < 0)
        {
            throw ERROR_INFO("invalid number of elements");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        num_elements = DefaultNumMinPointsForInference;
    }

    gpg_->SetNumPointsInference(num_elements);

    pConfig->Profile.SetInt(GetConfigPath() + "/gp_min_points_inference", num_elements);

    return error;
}

bool GuideAlgorithmGaussianProcess::SetNumPointsPeriodComputation(int num_points)
{
    bool error = false;

    try
    {
        if (num_points < 0)
        {
            throw ERROR_INFO("invalid number of points");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        num_points = DefaultNumMinPointsForPeriodComputation;
    }

    gpg_->SetNumPointsPeriodComputation(num_points);

    pConfig->Profile.SetInt(GetConfigPath() + "/gp_min_points_period_computation", num_points);

    return error;
}

bool GuideAlgorithmGaussianProcess::SetNumPointsForApproximation(int num_points)
{
    bool error = false;

    try
    {
        if (num_points < 0)
        {
            throw ERROR_INFO("invalid number of points");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        num_points = DefaultNumPointsForApproximation;
    }

    gpg_->SetNumPointsForApproximation(num_points);

    pConfig->Profile.SetInt(GetConfigPath() + "/gp_points_approximation", num_points);

    return error;
}

bool GuideAlgorithmGaussianProcess::SetGPHyperparameters(std::vector<double> hyperparameters)
{
    if(hyperparameters.size() != 8)
        return false;

    bool error = false;

    // we do this check in sequence: maybe there would be additional checks on this later.

    // gaussian process noise (dirac kernel)
    try
    {
        if (hyperparameters[0] < 0)
        {
            throw ERROR_INFO("invalid noise for dirac kernel");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters[0] = DefaultGaussianNoiseHyperparameter;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_gaussian_noise", hyperparameters[0]);

    // length scale short SE kernel
    try
    {
      if (hyperparameters[1] < 0)
      {
        throw ERROR_INFO("invalid length scale for short SE kernel");
      }
    }
    catch (wxString Msg)
    {
      POSSIBLY_UNUSED(Msg);
      error = true;
      hyperparameters[1] = DefaultLengthScaleSE0Ker;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_length_scale_se0_kern", hyperparameters[1]);

    // signal variance short SE kernel
    try
    {
      if (hyperparameters[2] < 0)
      {
        throw ERROR_INFO("invalid signal variance for the short SE kernel");
      }
    }
    catch (wxString Msg)
    {
      POSSIBLY_UNUSED(Msg);
      error = true;
      hyperparameters[2] = DefaultSignalVarianceSE0Ker;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_sigvar_se0_kern", hyperparameters[2]);

    // length scale periodic kernel
    try
    {
        if (hyperparameters[3] < 0)
        {
            throw ERROR_INFO("invalid length scale for periodic kernel");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters[3] = DefaultLengthScalePerKer;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_length_scale_per_kern", hyperparameters[3]);

    // signal variance periodic kernel
    try
    {
        if (hyperparameters[4] < 0)
        {
            throw ERROR_INFO("invalid signal variance for the periodic kernel");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters[4] = DefaultSignalVariancePerKer;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_sigvar_per_kern", hyperparameters[4]);


    // length scale long SE kernel
    try
    {
        if (hyperparameters[5] < 0)
        {
            throw ERROR_INFO("invalid length scale for SE kernel");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters[5] = DefaultLengthScaleSE1Ker;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_length_scale_se1_kern", hyperparameters[5]);

    // signal variance SE kernel
    try
    {
        if (hyperparameters[6] < 0)
        {
            throw ERROR_INFO("invalid signal variance for the SE kernel");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters[6] = DefaultSignalVarianceSE1Ker;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_sigvar_se1_kern", hyperparameters[6]);

    // period length periodic kernel
    try
    {
      if (hyperparameters[7] < 0)
      {
        throw ERROR_INFO("invalid period length for periodic kernel");
      }
    }
    catch (wxString Msg)
    {
      POSSIBLY_UNUSED(Msg);
      error = true;
      hyperparameters[7] = DefaultPeriodLengthPerKer;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_period_per_kern", hyperparameters[7]);

    gpg_->SetGPHyperparameters(hyperparameters);
    return error;
}

bool GuideAlgorithmGaussianProcess::SetPredictionGain(double prediction_gain)
{
    bool error = false;

    try
    {
        if (prediction_gain < 0 || prediction_gain > 1.0)
        {
            throw ERROR_INFO("invalid prediction gain");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        prediction_gain = DefaultPredictionGain;
    }

    gpg_->SetPredictionGain(prediction_gain);

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_prediction_gain", prediction_gain);

    return error;
}

bool GuideAlgorithmGaussianProcess::SetBoolComputePeriod(bool active)
{
  gpg_->SetBoolComputePeriod(active);
  pConfig->Profile.SetBoolean(GetConfigPath() + "/gp_compute_period", active);
  return true;
}

double GuideAlgorithmGaussianProcess::GetControlGain() const
{
    return gpg_->GetControlGain();
}

double GuideAlgorithmGaussianProcess::GetMinMove() const
{
    return gpg_->GetMinMove();
}

int GuideAlgorithmGaussianProcess::GetNumPointsInference() const
{
    return gpg_->GetNumPointsInference();
}

int GuideAlgorithmGaussianProcess::GetNumPointsPeriodComputation() const
{
    return gpg_->GetNumPointsPeriodComputation();
}

int GuideAlgorithmGaussianProcess::GetNumPointsForApproximation() const
{
    return gpg_->GetNumPointsForApproximation();
}

std::vector<double> GuideAlgorithmGaussianProcess::GetGPHyperparameters() const
{
    return gpg_->GetGPHyperparameters();
}

double GuideAlgorithmGaussianProcess::GetPredictionGain() const
{
    return gpg_->GetPredictionGain();
}

bool GuideAlgorithmGaussianProcess::GetBoolComputePeriod() const
{
    return gpg_->GetBoolComputePeriod();
}

bool GuideAlgorithmGaussianProcess::GetDarkTracking()
{
    return dark_tracking_mode_;
}

bool GuideAlgorithmGaussianProcess::SetDarkTracking(bool value)
{
    dark_tracking_mode_ = value;
    return false;
}

bool GuideAlgorithmGaussianProcess::GetExpertMode()
{
    return expert_mode_;
}

bool GuideAlgorithmGaussianProcess::SetExpertMode(bool value)
{
    expert_mode_ = value;
    return false;
}

wxString GuideAlgorithmGaussianProcess::GetSettingsSummary()
{
    static const char* format =
      "Control gain = %.3f\n"
      "Prediction gain = %.3f\n"
      "Minimum move = %.3f\n"
      "Hyperparameters\n"
      "\tLength scale long range SE kernel = %.3f\n"
      "\tSignal variance long range SE kernel = %.3f\n"
      "\tLength scale periodic kernel = %.3f\n"
      "\tSignal variance periodic kernel = %.3f\n"
      "\tLength scale short range SE kernel = %.3f\n"
      "\tSignal variance short range SE kernel = %.3f\n"
      "\tPeriod length periodic kernel = %.3f\n"
      "FFT called after = %d points\n"
    ;

    std::vector<double> hyperparameters = GetGPHyperparameters();

    return wxString::Format(
      format,
      GetControlGain(),
      GetPredictionGain(),
      GetMinMove(),
      hyperparameters[1],
      hyperparameters[2],
      hyperparameters[3],
      hyperparameters[4],
      hyperparameters[5],
      hyperparameters[6],
      hyperparameters[7],
      GetNumPointsPeriodComputation());
}


GUIDE_ALGORITHM GuideAlgorithmGaussianProcess::Algorithm(void)
{
    return GUIDE_ALGORITHM_GAUSSIAN_PROCESS;
}

double GuideAlgorithmGaussianProcess::result(double input)
{
    if (dark_tracking_mode_ == true)
    {
        return deduceResult();
    }

    // the third parameter of result() is a floating-point in seconds, while RequestedExposureDuration() returns milliseconds
    double control_signal = gpg_->result(input, pFrame->pGuider->SNR(), (double) pFrame->RequestedExposureDuration()/1000.0);

    Debug.AddLine(wxString::Format("Predictive PEC Guider: input: %f, control: %f, exposure: %f",
        input, control_signal, (double) pFrame->RequestedExposureDuration()/1000.0));

    assert(std::abs(control_signal) < 100); // such large control signals don't make sense
    assert(!math_tools::isNaN(control_signal));
    return control_signal;
}

double GuideAlgorithmGaussianProcess::deduceResult()
{
    double control_signal = gpg_->deduceResult((double) pFrame->RequestedExposureDuration()/1000.0);

    Debug.AddLine(wxString::Format("Predictive PEC Guider (deduced): control: %f, exposure: %f", control_signal,
        (double) pFrame->RequestedExposureDuration()/1000.0));

    assert(std::abs(control_signal) < 100); // such large control signals don't make sense
    assert(!math_tools::isNaN(control_signal));
    return control_signal;
}

void GuideAlgorithmGaussianProcess::reset()
{
    gpg_->reset();
}

void GuideAlgorithmGaussianProcess::GuidingStopped(void)
{
    reset(); // reset is only done on a complete stop
}

void GuideAlgorithmGaussianProcess::GuidingPaused(void)
{
}

void GuideAlgorithmGaussianProcess::GuidingResumed(void)
{
}

void GuideAlgorithmGaussianProcess::GuidingDithered(double amt)
{
    CalibrationDetails calDetails;
    m_pMount->GetCalibrationDetails(&calDetails);

    double guide_speed = 1.0; // normalized to 15 a-s per second
                              // 1.0 means 15 a-s/sec,
                              // 0.5 means 7.5 a-s/sec, etc.

    if (calDetails.raGuideSpeed != -1.0)
    {
        guide_speed = 3600.0 * calDetails.raGuideSpeed / 15.0; // normalize!
    }

    // the guide rate here is normalized to seconds and adjusted for the speed
    double guide_rate = 1000 * m_pMount->xRate() / guide_speed;

    // just hand it on to the guide algorithm, and pass the RA rate
    gpg_->GuidingDithered(amt, guide_rate);
}

void GuideAlgorithmGaussianProcess::GuidingDitherSettleDone(bool success)
{
    // just hand it on to the guide algorithm
    gpg_->GuidingDitherSettleDone(success);
}
