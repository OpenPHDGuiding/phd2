/**
 * PHD2 Guiding
 *
 * @file
 * @date      2014-2016
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

class GuideAlgorithmGaussianProcess::GuideAlgorithmGaussianProcessDialogPane : public wxEvtHandler, public ConfigDialogPane
{
    GuideAlgorithmGaussianProcess *m_pGuideAlgorithm;
    wxSpinCtrlDouble *m_pControlGain;
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

    wxBoxSizer *m_pExpertPage;

public:
    GuideAlgorithmGaussianProcessDialogPane(wxWindow *pParent, GuideAlgorithmGaussianProcess *pGuideAlgorithm)
      : ConfigDialogPane(_("Gaussian Process Guide Algorithm"),pParent)
    {
        m_pGuideAlgorithm = pGuideAlgorithm;

        int width = StringWidth(_T("000.00"));
        m_pControlGain = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, _T("foo2"),
            wxPoint(-1, -1), wxSize(width, -1),
                                              wxSP_ARROW_KEYS, 0.0, 1.0, 0.0, 0.05,
                                              _T("Control Gain"));
        m_pControlGain->SetDigits(2);
        DoAdd(_("Control gain"), m_pControlGain,
              _("The control gain defines how aggressive the controller is. It is the amount of pointing error that is "
              "fed back to the system. Default = 0.8"));

        m_pPKPeriodLength = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                 wxDefaultPosition,wxSize(width+30, -1),
                                                 wxSP_ARROW_KEYS, 50, 2000, 500, 1);
        m_pPKPeriodLength->SetDigits(2);
        m_checkboxComputePeriod = new wxCheckBox(pParent, wxID_ANY, _T("auto"));
        DoAdd(_("Period length [periodic]"), m_pPKPeriodLength,
              _("The period length of the periodic error component that should be corrected. Default = 500.0"), m_checkboxComputePeriod);

        // number of points used for the approximate GP inference (subset of data)
        m_pNumPointsApproximation = new wxSpinCtrl(pParent, wxID_ANY, wxEmptyString,
                                                   wxDefaultPosition,wxSize(width+30, -1),
                                                   wxSP_ARROW_KEYS, 0, 2000, 10);
        DoAdd(_("Used data points (approximation)"), m_pNumPointsApproximation,
              _("Number of data points used in the approximation. Both prediction accuracy as well as runtime rise with "
              "the number of datapoints. Default = 100"));

        // create the expert options page
        m_pExpertPage = new wxBoxSizer(wxVERTICAL);

        m_pPredictionGain = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                  wxDefaultPosition,wxSize(width+30, -1),
                                                  wxSP_ARROW_KEYS, 0.0, 1.0, 0.8, 0.01);
        m_pPredictionGain->SetDigits(2);
        m_pExpertPage->Add(MakeLabeledControl(_("Prediction gain"), m_pPredictionGain,
              _("The prediction gain defines how much control signal is generated from the prediction. Default = 1.0")));

        // number of elements before starting the inference
        m_pNumPointsInference = new wxSpinCtrl(pParent, wxID_ANY, wxEmptyString,
                                             wxDefaultPosition,wxSize(width+30, -1),
                                             wxSP_ARROW_KEYS, 0, 1000, 10);
        m_pExpertPage->Add(MakeLabeledControl(_("Minimum data points (inference)"), m_pNumPointsInference,
              _("Minimal number of measurements to start using the Gaussian process. If there are too little data points, "
              "the result might be poor. Default = 25")));

        m_pNumPointsPeriodComputation = new wxSpinCtrl(pParent, wxID_ANY, wxEmptyString,
                                                 wxDefaultPosition,wxSize(width+30, -1),
                                                 wxSP_ARROW_KEYS, 0, 1000, 10);
        m_pExpertPage->Add(MakeLabeledControl(_("Minimum data points (period)"), m_pNumPointsPeriodComputation,
              _("Minimal number of measurements to start estimating the periodicity. If there are too little data points, "
              "the estimation might not work. Default = 100")));

        // hyperparameters
        m_pSE0KLengthScale = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                 wxDefaultPosition,wxSize(width+30, -1),
                                                 wxSP_ARROW_KEYS, 0.0, 5000.0, 500.0, 1.0);
        m_pSE0KLengthScale->SetDigits(2);
        m_pExpertPage->Add(MakeLabeledControl(_("Length scale [long range]"), m_pSE0KLengthScale,
              _("The length scale of the large non-periodic structure in the error. This is essentially a high-pass "
              "filter and the length scale defines the corner frequency. Default = 500.0")));

        m_pSE0KSignalVariance = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                    wxDefaultPosition,wxSize(width+30, -1),
                                                    wxSP_ARROW_KEYS, 0.0, 10, 1, 0.1);
        m_pSE0KSignalVariance->SetDigits(2);
        m_pExpertPage->Add(MakeLabeledControl(_("Signal Variance [long range]"), m_pSE0KSignalVariance,
              _("Signal Variance of the long-term variations. Default = 10.0")));


        m_pPKLengthScale = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                wxDefaultPosition,wxSize(width+30, -1),
                                                wxSP_ARROW_KEYS, 0.0, 10, 1.0, 0.1);
        m_pPKLengthScale->SetDigits(2);
        m_pExpertPage->Add(MakeLabeledControl(_("Length scale [periodic]"), m_pPKLengthScale,
              _("The length scale defines the \"wigglyness\" of the function. The smaller the length scale, the more "
              "structure can be learned. If chosen too small, some non-periodic structure might be picked up as well. "
              "Default = 0.5")));



        m_pPKSignalVariance = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                   wxDefaultPosition,wxSize(width+30, -1),
                                                   wxSP_ARROW_KEYS, 0.0, 30, 10, 0.1);
        m_pPKSignalVariance->SetDigits(2);
        m_pExpertPage->Add(MakeLabeledControl(_("Signal variance [periodic]"), m_pPKSignalVariance,
              _("The width of the periodic error. Should be around the amplitude of the PE curve, but is not a critical parameter. "
              "Default = 10.0")));

        m_pSE1KLengthScale = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                 wxDefaultPosition,wxSize(width+30, -1),
                                                 wxSP_ARROW_KEYS, 0.0, 10.0, 5.0, 0.1);
        m_pSE1KLengthScale->SetDigits(2);
        m_pExpertPage->Add(MakeLabeledControl(_("Length scale [short range]"), m_pSE1KLengthScale,
              _("The length scale of the short range non-periodic parts of the gear error. This is essentially a low-pass "
              "filter and the length scale defines the corner frequency. Default = 5.0")));

        m_pSE1KSignalVariance = new wxSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString,
                                                 wxDefaultPosition,wxSize(width+30, -1),
                                                 wxSP_ARROW_KEYS, 0.0, 10, 1, 0.1);
        m_pSE1KSignalVariance->SetDigits(2);
        m_pExpertPage->Add(MakeLabeledControl(_("Signal Variance [short range]"), m_pSE1KSignalVariance,
              _("Signal Variance of the short-term variations. Default = 1.0")));



        m_checkboxExpertMode = new wxCheckBox(pParent, wxID_ANY, _T(""));
        pParent->Connect(m_checkboxExpertMode->GetId(), wxEVT_CHECKBOX, wxCommandEventHandler(GuideAlgorithmGaussianProcess::GuideAlgorithmGaussianProcessDialogPane::EnableExpertMode), 0, this);
        DoAdd(_("Show expert options"), m_checkboxExpertMode, _("This is just for debugging and disabled by default"));



        m_checkboxDarkMode = new wxCheckBox(pParent, wxID_ANY, "");
        m_pExpertPage->Add(MakeLabeledControl(_("Dark guiding mode"), m_checkboxDarkMode, _("This is just for debugging and disabled by default")));

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

        m_pPredictionGain->SetValue(m_pGuideAlgorithm->GetPredictionGain());

        m_checkboxComputePeriod->SetValue(m_pGuideAlgorithm->GetBoolComputePeriod());

        m_pExpertPage->ShowItems(m_pGuideAlgorithm->GetExpertMode());
        m_pExpertPage->Layout();
        m_pParent->Layout();
    }

    // Set the parameters chosen in the GUI in the actual guiding algorithm
    virtual void UnloadValues(void)
    {
        m_pGuideAlgorithm->SetControlGain(m_pControlGain->GetValue());
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
        m_pGuideAlgorithm->SetPredictionGain(m_pPredictionGain->GetValue());
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


struct gp_data_point
{
    double timestamp;
    double measurement; // current pointing error
    double variance; // current measurement variance
    double control; // control action
};


// data structure of the GP guiding algorithm
struct GuideAlgorithmGaussianProcess::gp_guide_parameters
{
    circular_buffer<gp_data_point> circular_buffer_data;

    wxStopWatch timer_;
    double control_signal_;
    double control_gain_;
    double last_timestamp_;
    double prediction_gain_;
    double prediction_;
    double last_prediction_end_;

    int min_points_for_inference_;
    int min_points_for_period_computation_;

    int points_for_approximation_;

    bool compute_period_;
    bool dark_tracking_mode_;
    bool expert_mode_;

    bool dithering_active_;
    int dither_steps_;

    covariance_functions::PeriodicSquareExponential2 covariance_function_; // for inference
    covariance_functions::PeriodicSquareExponential output_covariance_function_; // for prediction
    GP gp_;

    gp_guide_parameters() :
      circular_buffer_data(CIRCULAR_BUFFER_SIZE),
      timer_(),
      control_signal_(0.0),
      control_gain_(0.0),
      last_timestamp_(0.0),
      prediction_gain_(0.0),
      prediction_(0.0),
      last_prediction_end_(0.0),
      min_points_for_inference_(0),
      min_points_for_period_computation_(0),
      points_for_approximation_(0),
      compute_period_(false),
      dark_tracking_mode_(false),
      expert_mode_(false),
      dithering_active_(false),
      dither_steps_(0),
      gp_(covariance_function_)
    {
        circular_buffer_data.push_front(gp_data_point()); // add first point
        circular_buffer_data[0].control = 0; // set first control to zero
        gp_.enableOutputProjection(output_covariance_function_); // for prediction
    }

    gp_data_point& get_last_point()
    {
        return circular_buffer_data[circular_buffer_data.size() - 1];
    }

    gp_data_point& get_second_last_point()
    {
        return circular_buffer_data[circular_buffer_data.size() - 2];
    }

    size_t get_number_of_measurements() const
    {
        return circular_buffer_data.size();
    }

    void add_one_point()
    {
        circular_buffer_data.push_front(gp_data_point());
    }

    void clear()
    {
        circular_buffer_data.clear();
        circular_buffer_data.push_front(gp_data_point()); // add first point
        circular_buffer_data[0].control = 0; // set first control to zero
        last_prediction_end_ = 0.0;
        gp_.clearData();
    }

};


static const double DefaultControlGain                   = 0.8; // control gain
static const int    DefaultNumMinPointsForInference       = 25; // minimal number of points for doing the inference

static const double DefaultGaussianNoiseHyperparameter   = 1.0; // default Gaussian measurement noise

static const double DefaultLengthScaleSE0Ker             = 500.0; // length-scale of the long-range SE-kernel
static const double DefaultSignalVarianceSE0Ker          = 10.0; // signal variance of the long-range SE-kernel
static const double DefaultLengthScalePerKer             = 0.5; // length-scale of the periodic kernel
static const double DefaultPeriodLengthPerKer            = 500; // P_p, period-length of the periodic kernel
static const double DefaultSignalVariancePerKer          = 10.0; // signal variance of the periodic kernel
static const double DefaultLengthScaleSE1Ker             = 5.0; // length-scale of the short-range SE-kernel
static const double DefaultSignalVarianceSE1Ker          = 1.0; // signal variance of the short range SE-kernel

static const int    DefaultNumMinPointsForPeriodComputation = 100; // minimal number of points for doing the period identification
static const int    DefaultNumPointsForApproximation        = 100; // number of points used in the GP approximation
static const double DefaultPredictionGain                  = 1.0; // amount of GP prediction to blend in

static const bool   DefaultComputePeriod                 = true;

GuideAlgorithmGaussianProcess::GuideAlgorithmGaussianProcess(Mount *pMount, GuideAxis axis)
    : GuideAlgorithm(pMount, axis), parameters(0)
{
    parameters = new gp_guide_parameters();
    wxString configPath = GetConfigPath();

    double control_gain = pConfig->Profile.GetDouble(configPath + "/gp_control_gain", DefaultControlGain);
    SetControlGain(control_gain);

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

    parameters->gp_.enableExplicitTrend(); // enable the explicit basis function for the linear drift
    parameters->dark_tracking_mode_ = false; // dark tracking mode ignores measurements
    parameters->expert_mode_ = false; // expert mode exposes the GP hyperparameters
    reset();
}

GuideAlgorithmGaussianProcess::~GuideAlgorithmGaussianProcess(void)
{
    delete parameters;
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

        parameters->control_gain_ = control_gain;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        parameters->control_gain_ = DefaultControlGain;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_control_gain", parameters->control_gain_);

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

        parameters->min_points_for_inference_ = num_elements;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        parameters->min_points_for_inference_ = DefaultNumMinPointsForInference;
    }

    pConfig->Profile.SetInt(GetConfigPath() + "/gp_min_points_inference", parameters->min_points_for_inference_);

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

        parameters->min_points_for_period_computation_ = num_points;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        parameters->min_points_for_period_computation_ = DefaultNumMinPointsForPeriodComputation;
    }

    pConfig->Profile.SetInt(GetConfigPath() + "/gp_min_points_period_computation", parameters->min_points_for_period_computation_);

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

        parameters->points_for_approximation_ = num_points;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        parameters->points_for_approximation_ = DefaultNumPointsForApproximation;
    }

    pConfig->Profile.SetInt(GetConfigPath() + "/gp_points_approximation", parameters->points_for_approximation_);

    return error;
}

bool GuideAlgorithmGaussianProcess::SetGPHyperparameters(std::vector<double> const &hyperparameters)
{
    if(hyperparameters.size() != 8)
        return false;

    Eigen::VectorXd hyperparameters_eig = Eigen::VectorXd::Map(&hyperparameters[0], hyperparameters.size());
    bool error = false;

    // we do this check in sequence: maybe there would be additional checks on this later.

    // gaussian process noise (dirac kernel)
    try
    {
        if (hyperparameters_eig(0) < 0)
        {
            throw ERROR_INFO("invalid noise for dirac kernel");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters_eig(0) = DefaultGaussianNoiseHyperparameter;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_gaussian_noise", hyperparameters_eig(0));

    // length scale short SE kernel
    try
    {
      if (hyperparameters_eig(1) < 0)
      {
        throw ERROR_INFO("invalid length scale for short SE kernel");
      }
    }
    catch (wxString Msg)
    {
      POSSIBLY_UNUSED(Msg);
      error = true;
      hyperparameters_eig(1) = DefaultLengthScaleSE0Ker;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_length_scale_se0_kern", hyperparameters_eig(1));

    // signal variance short SE kernel
    try
    {
      if (hyperparameters_eig(2) < 0)
      {
        throw ERROR_INFO("invalid signal variance for the short SE kernel");
      }
    }
    catch (wxString Msg)
    {
      POSSIBLY_UNUSED(Msg);
      error = true;
      hyperparameters_eig(2) = DefaultSignalVarianceSE0Ker;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_sigvar_se0_kern", hyperparameters_eig(2));

    // length scale periodic kernel
    try
    {
        if (hyperparameters_eig(3) < 0)
        {
            throw ERROR_INFO("invalid length scale for periodic kernel");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters_eig(3) = DefaultLengthScalePerKer;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_length_scale_per_kern", hyperparameters_eig(3));

    // signal variance periodic kernel
    try
    {
        if (hyperparameters_eig(4) < 0)
        {
            throw ERROR_INFO("invalid signal variance for the periodic kernel");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters_eig(4) = DefaultSignalVariancePerKer;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_sigvar_per_kern", hyperparameters_eig(4));


    // length scale long SE kernel
    try
    {
        if (hyperparameters_eig(5) < 0)
        {
            throw ERROR_INFO("invalid length scale for SE kernel");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters_eig(5) = DefaultLengthScaleSE1Ker;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_length_scale_se1_kern", hyperparameters_eig(5));

    // signal variance SE kernel
    try
    {
        if (hyperparameters_eig(6) < 0)
        {
            throw ERROR_INFO("invalid signal variance for the SE kernel");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        hyperparameters_eig(6) = DefaultSignalVarianceSE1Ker;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_sigvar_se1_kern", hyperparameters_eig(6));

    // period length periodic kernel
    try
    {
      if (hyperparameters_eig(7) < 0)
      {
        throw ERROR_INFO("invalid period length for periodic kernel");
      }
    }
    catch (wxString Msg)
    {
      POSSIBLY_UNUSED(Msg);
      error = true;
      hyperparameters_eig(7) = DefaultPeriodLengthPerKer;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_period_per_kern", hyperparameters_eig(7));

    // note that the GP class works in log space! Thus, we have to convert the parameters to log.
    parameters->gp_.setHyperParameters(hyperparameters_eig.array().log());
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

        parameters->prediction_gain_ = prediction_gain;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
        parameters->prediction_gain_ = DefaultPredictionGain;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/gp_prediction_gain", parameters->prediction_gain_);

    return error;
}

bool GuideAlgorithmGaussianProcess::SetBoolComputePeriod(bool active)
{
  parameters->compute_period_ = active;
  pConfig->Profile.SetBoolean(GetConfigPath() + "/gp_compute_period", parameters->compute_period_);
  return true;
}

double GuideAlgorithmGaussianProcess::GetControlGain() const
{
    return parameters->control_gain_;
}

int GuideAlgorithmGaussianProcess::GetNumPointsInference() const
{
    return parameters->min_points_for_inference_;
}

int GuideAlgorithmGaussianProcess::GetNumPointsPeriodComputation() const
{
    return parameters->min_points_for_period_computation_;
}

int GuideAlgorithmGaussianProcess::GetNumPointsForApproximation() const
{
    return parameters->points_for_approximation_;
}

std::vector<double> GuideAlgorithmGaussianProcess::GetGPHyperparameters() const
{
    // since the GP class works in log space, we have to exp() the parameters first.
    Eigen::VectorXd hyperparameters = parameters->gp_.getHyperParameters().array().exp();

    // we need to map the Eigen::vector into a std::vector.
    return std::vector<double>(hyperparameters.data(), // the first element is at the array address
                               hyperparameters.data() + 8); // 8 parameters, therefore the last is at position 7
}

double GuideAlgorithmGaussianProcess::GetPredictionGain() const
{
    return parameters->prediction_gain_;
}

bool GuideAlgorithmGaussianProcess::GetBoolComputePeriod() const
{
    return parameters->compute_period_;
}

bool GuideAlgorithmGaussianProcess::GetDarkTracking()
{
    return parameters->dark_tracking_mode_;
}

bool GuideAlgorithmGaussianProcess::SetDarkTracking(bool value)
{
    parameters->dark_tracking_mode_ = value;
    return false;
}

bool GuideAlgorithmGaussianProcess::GetExpertMode()
{
    return parameters->expert_mode_;
}

bool GuideAlgorithmGaussianProcess::SetExpertMode(bool value)
{
    parameters->expert_mode_ = value;
    return false;
}

wxString GuideAlgorithmGaussianProcess::GetSettingsSummary()
{
    static const char* format =
      "Control Gain = %.3f\n"
      "Prediction gain = %.3f\n"
      "Hyperparameters\n"
      "\tLength scale long range SE kernel = %.3f\n"
      "\tSignal variance long range SE kernel = %.3f\n"
      "\tLength scale periodic kernel = %.3f\n"
      "\tSignal variance periodic kernel = %.3f\n"
      "\tLength scale short range SE kernel = %.3f\n"
      "\tSignal variance short range SE kernel = %.3f\n"
      "\tPeriod length periodic kernel = %.3f\n"
      "FFT called every = %.3d points\n"
    ;

    Eigen::VectorXd hyperparameters = parameters->gp_.getHyperParameters();

    return wxString::Format(
      format,
      GetControlGain(),
      parameters->prediction_gain_,
      std::exp(hyperparameters(0)), std::exp(hyperparameters(1)),
      std::exp(hyperparameters(2)), std::exp(hyperparameters(3)),
      std::exp(hyperparameters(4)), std::exp(hyperparameters(5)),
      std::exp(hyperparameters(6)), std::exp(hyperparameters(7)),
      parameters->min_points_for_period_computation_);
}


GUIDE_ALGORITHM GuideAlgorithmGaussianProcess::Algorithm(void)
{
    return GUIDE_ALGORITHM_GAUSSIAN_PROCESS;
}

void GuideAlgorithmGaussianProcess::HandleTimestamps()
{
    if (parameters->get_number_of_measurements() == 0)
    {
        parameters->timer_.Start();
    }
    double time_now = parameters->timer_.Time();
    double delta_measurement_time_ms = time_now - parameters->last_timestamp_;
    parameters->last_timestamp_ = time_now;
    parameters->get_last_point().timestamp = (time_now - delta_measurement_time_ms / 2.0) / 1000.0;
}

// adds a new measurement to the circular buffer that holds the data.
void GuideAlgorithmGaussianProcess::HandleMeasurements(double input)
{
    parameters->get_last_point().measurement = input;
}

void GuideAlgorithmGaussianProcess::HandleDarkGuiding()
{
    parameters->get_last_point().measurement = 0; // we didn't actually measure
    parameters->get_last_point().variance = 1e4; // add really high noise
}

void GuideAlgorithmGaussianProcess::HandleControls(double control_input)
{
    parameters->get_last_point().control = control_input;
}

void GuideAlgorithmGaussianProcess::HandleSNR(double SNR)
{
    SNR = std::max(SNR, 3.4); // limit the minimal SNR

    // this was determined by simulated experiments
    double standard_deviation = 2.1752 * 1 / (SNR - 3.3) + 0.5;

    parameters->get_last_point().variance = standard_deviation * standard_deviation;
}

void GuideAlgorithmGaussianProcess::UpdateGP()
{
    clock_t begin = std::clock(); // this is for timing the method in a simple way

    int N = parameters->get_number_of_measurements();

    // initialize the different vectors needed for the GP
    Eigen::VectorXd timestamps(N-1);
    Eigen::VectorXd measurements(N-1);
    Eigen::VectorXd variances(N-1);
    Eigen::VectorXd sum_controls(N-1);

    double sum_control = 0;

    // transfer the data from the circular buffer to the Eigen::Vectors
    for(size_t i = 0; i < N-1; i++)
    {
        sum_control += parameters->circular_buffer_data[i].control; // sum over the control signals
        timestamps(i) = parameters->circular_buffer_data[i].timestamp;
        measurements(i) = parameters->circular_buffer_data[i].measurement;
        variances(i) = parameters->circular_buffer_data[i].variance;
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
    if (parameters->compute_period_ && parameters->min_points_for_period_computation_ > 0
      && parameters->get_number_of_measurements() > parameters->min_points_for_period_computation_)
    {
      // find periodicity parameter with FFT

      // compute Hamming window to reduce spectral leakage
      Eigen::VectorXd windowed_gear_error = gear_error_detrend.array() * math_tools::hamming_window(gear_error_detrend.rows()).array();

      // compute the spectrum
      std::pair<Eigen::VectorXd, Eigen::VectorXd> result = math_tools::compute_spectrum(windowed_gear_error, FFT_SIZE);

      Eigen::ArrayXd amplitudes = result.first;
      Eigen::ArrayXd frequencies = result.second;

      double dt = (timestamps(timestamps.rows()-1) - timestamps(0))/timestamps.rows(); // (t_end - t_begin) / num_t
      if (dt < 0)
      {
          Debug.AddLine("Something is wrong: The average time step length is is negative!");
          Debug.AddLine(wxString::Format("timestamps: last: %f, first: %f, rows: %f", timestamps(timestamps.rows() - 1), timestamps(1), timestamps.rows()));
          return;
      }

      frequencies /= dt; // correct for the average time step width

      Eigen::ArrayXd periods = 1/frequencies.array();
      amplitudes = (periods > 1500.0).select(0,amplitudes); // set amplitudes to zero for too large periods

      assert(amplitudes.size() == frequencies.size());

      Eigen::VectorXd::Index maxIndex;
      amplitudes.maxCoeff(&maxIndex);
      double period_length = 1 / frequencies(maxIndex);

      Eigen::VectorXd hypers = parameters->gp_.getHyperParameters();
      hypers[7] = std::log(period_length); // parameters are stored in log space
      parameters->gp_.setHyperParameters(hypers);

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
    parameters->gp_.inferSD(timestamps, gear_error, parameters->points_for_approximation_, variances, parameters->timer_.Time() / 1000.0);

    end = std::clock();
    double time_gp = double(end - begin) / CLOCKS_PER_SEC;
    Debug.AddLine(wxString::Format("timings: init: %f, detrend: %f, fft: %f, gp: %f", time_init, time_detrend, time_fft, time_gp));
}

double GuideAlgorithmGaussianProcess::PredictGearError()
{
    int delta_controller_time_ms = pFrame->RequestedExposureDuration();

    // prevent large jumps after calling clear()
    if ( parameters->last_prediction_end_ < 1.0 )
    {
        parameters->last_prediction_end_ = parameters->timer_.Time() / 1000.0;
    }

    // prediction from the last endpoint to the prediction point
    Eigen::VectorXd next_location(2);
    next_location << parameters->last_prediction_end_,
    (parameters->timer_.Time() + delta_controller_time_ms) / 1000.0;
    Eigen::VectorXd prediction = parameters->gp_.predictProjected(next_location).first;

    double p1 = prediction(1);
    double p0 = prediction(0);
    assert(std::abs(p1 - p0) < 100); // large differences don't make sense
    assert(!math_tools::isNaN(p1 - p0));

    parameters->last_prediction_end_ = next_location(1); // store current endpoint

    // we are interested in the error introduced by the gear over the next time step
    return (p1 - p0);
}

double GuideAlgorithmGaussianProcess::result(double input)
{
    if (parameters->dark_tracking_mode_ == true)
    {
        return deduceResult();
    }

    /*
    * Dithering behaves differently from pausing. During dithering, the mount
    * is moved and thus we can assume that we applied the perfect control, but
    * we cannot trust the measurement. Once dithering has settled, we can trust
    * the measurement again and we can pretend nothing has happend.
    */
    if (parameters->dithering_active_ == true)
    {
        parameters->dither_steps_--;
        if (parameters->dither_steps_ <= 0)
        {
            parameters->dithering_active_ = false;
        }
        deduceResult(); // just pretend we would do dark guiding...
        return parameters->control_gain_*input; // ...but apply proportional control
    }

    // collect data point content, except for the control signal
    HandleMeasurements(input);
    HandleTimestamps();
    HandleSNR(pFrame->pGuider->SNR());

    parameters->control_signal_ = parameters->control_gain_*input; // start with proportional control

    // check if we are allowed to use the GP
    if (parameters->min_points_for_inference_ > 0 &&
        parameters->get_number_of_measurements() > parameters->min_points_for_inference_)
    {
        UpdateGP(); // update the GP based on the new measurements
        parameters->prediction_ = PredictGearError();
        parameters->control_signal_ += parameters->prediction_gain_*parameters->prediction_; // add the prediction
    }

    parameters->add_one_point(); // add new point here, since the control is for the next point in time
    HandleControls(parameters->control_signal_); // already store control signal

// write the GP output to a file for easy analyzation
#if GP_DEBUG_FILE_
    int N = parameters->get_number_of_measurements();

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
        timestamps(i) = parameters->circular_buffer_data[i].timestamp;
        measurements(i) = parameters->circular_buffer_data[i].measurement;
        variances(i) = parameters->circular_buffer_data[i].variance;
        controls(i) = parameters->circular_buffer_data[i].control;
        sum_controls(i) = parameters->circular_buffer_data[i].control;
        if(i > 0)
        {
            sum_controls(i) += sum_controls(i-1); // sum over the control signals
        }
    }
    gear_error = sum_controls + measurements; // for each time step, add the residual error

    // inference of the GP with these new points
    parameters->gp_.inferSD(timestamps, gear_error, parameters->points_for_approximation_, variances, parameters->timer_.Time() / 1000.0);

    int M = 512; // number of prediction points
    Eigen::VectorXd locations = Eigen::VectorXd::LinSpaced(M, 0, parameters->get_second_last_point().timestamp + 1500);

    std::pair<Eigen::VectorXd, Eigen::MatrixXd> predictions = parameters->gp_.predictProjected(locations);

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
    Debug.AddLine(wxString::Format("GP Guider generated %f from input %f.", parameters->control_signal_, input));

    assert(std::abs(parameters->control_signal_) < 100); // such large control signals don't make sense
    assert(!math_tools::isNaN(parameters->control_signal_));
    return parameters->control_signal_;
}

double GuideAlgorithmGaussianProcess::deduceResult()
{
    HandleDarkGuiding();
    HandleTimestamps();

    parameters->control_signal_ = 0; // no measurement!
    // check if we are allowed to use the GP
    if (parameters->min_points_for_inference_ > 0 &&
        parameters->get_number_of_measurements() > parameters->min_points_for_inference_)
    {
        UpdateGP(); // update the GP to update the SD approximation
        parameters->prediction_ = PredictGearError();
        parameters->control_signal_ += parameters->prediction_; // control based on prediction
    }

    parameters->add_one_point(); // add new point here, since the control is for the next point in time
    HandleControls(parameters->control_signal_); // already store control signal

    // write the GP output to a file for easy analyzation
#if GP_DEBUG_FILE_
    int N = parameters->get_number_of_measurements();

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
        timestamps(i) = parameters->circular_buffer_data[i].timestamp;
        measurements(i) = parameters->circular_buffer_data[i].measurement;
        variances(i) = parameters->circular_buffer_data[i].variance;
        controls(i) = parameters->circular_buffer_data[i].control;
        sum_controls(i) = parameters->circular_buffer_data[i].control;
        if (i > 0)
        {
            sum_controls(i) += sum_controls(i - 1); // sum over the control signals
        }
    }
    gear_error = sum_controls + measurements; // for each time step, add the residual error

    // inference of the GP with these new points
    parameters->gp_.inferSD(timestamps, gear_error, parameters->points_for_approximation_, variances, parameters->timer_.Time() / 1000.0);

    int M = 512; // number of prediction points
    Eigen::VectorXd locations = Eigen::VectorXd::LinSpaced(M, 0, parameters->get_second_last_point().timestamp + 1500);

    std::pair<Eigen::VectorXd, Eigen::MatrixXd> predictions = parameters->gp_.predictProjected(locations);

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

    assert(!math_tools::isNaN(parameters->control_signal_));
    return parameters->control_signal_;
}

void GuideAlgorithmGaussianProcess::reset()
{
    parameters->clear();
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
    /**
     * During a pause, the following happens: The no guiding commands are
     * issued, therefore the error builds up. Since we need this erorr to show
     * up in the data for the GP and for the FFT, we have to hallucinate the
     * error from the GP predictions. Note that this error needs to be
     * "measured" and not "controlled", since we didn't move the telescope.
     */

    int delta_controller_time_ms = pFrame->RequestedExposureDuration();

    // we need a fake measurement for every missed real measurement
    while (parameters->timer_.Time() - parameters->last_prediction_end_*1000
                > delta_controller_time_ms)
    {
        HandleMeasurements(PredictGearError());
        HandleTimestamps();
        HandleSNR(1e2); // assume high noise, since we didn't really measure

        parameters->control_signal_ = 0; // we didn't issue control signals

        parameters->add_one_point(); // add new point here, since the control is for the next point in time
        HandleControls(parameters->control_signal_);
    }
}

void GuideAlgorithmGaussianProcess::GuidingDithered(double amt)
{
    /*
     * We don't compensate for the dither amout (yet), but we need to know
     * that we are currently dithering.
     */
    parameters->dithering_active_ = true;
    parameters->dither_steps_ = 10;
}

void GuideAlgorithmGaussianProcess::GuidingDitherSettleDone(bool success)
{
    /*
     * Once dithering has settled, we can start regular guiding again.
     */
    if (success)
    {
        parameters->dithering_active_ = false;
        parameters->dither_steps_ = 0;
    }
}
